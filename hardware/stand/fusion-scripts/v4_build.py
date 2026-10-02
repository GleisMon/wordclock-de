# WordClockStand v4 — v3 + rounded (stadium) USB-C socket cutout, end wall thinned to 1.5 mm around it
import adsk.core, adsk.fusion, math

TILT = 10.0
CLOCK_T, CLOCK_W, CLR_END = 15.0, 187.12, 0.5
LIP_T, LIP_H = 4.0, 7.0
CLAMP, CB_DEPTH = 30.0, 2.5
REAR_N = CLAMP + CB_DEPTH       # 32.5: flat rear face for the screw heads
REAR_FLAT_U = 14.0              # flat rear face up to here (counterbore reaches u=10.3)
TOP_U = 52.0                    # rear wall meets the clock back here
WALL = 2.5
FLOOR_Y = 3.0
END_T = 3.0
BLOCK_Z0, BLOCK_U = 78.0, 12.0  # solid screw blocks at the ends (below the LED-wire slot at u>=13.7)
HOLE_U, HOLE_Z = 7.0, 86.56
D_THRU, D_CB = 4.2, 6.6
SOCK_N, SOCK_U, SOCK_WN, SOCK_WU = 11.0, 21.0, 13.9, 5.3
POCKET_WN, POCKET_WU, PANEL_T = 20.0, 11.0, 1.5   # inner pocket so the clips see a 1.5 mm panel
   # panel USB-C socket 13.7 x 5.1 (+0.2) in +z end wall, long side along n
YB = 5.0

c, s = math.cos(math.radians(TILT)), math.sin(math.radians(TILT))
u1 = -(YB + (CLOCK_T + LIP_T) * s) / c
XB = -(-(CLOCK_T + LIP_T) * c + u1 * s)
Z_IN = (CLOCK_W + 2 * CLR_END) / 2

def g(n, u): return (XB + n * c + u * s, YB - n * s + u * c)
def nu(x, y):  # inverse
    dx, dy = x - XB, y - YB
    return (dx * c - dy * s, dx * s + dy * c)
def at_y(n, y):  # point on line n=const at global height y
    return g(n, (y - YB + n * s) / c)

# inner slope: outer line (REAR_N, REAR_FLAT_U)->(0, TOP_U) offset WALL inwards
dn, du = -REAR_N, TOP_U - REAR_FLAT_U
L = math.hypot(dn, du)
nn, nu_ = -du / L, dn / L            # left normal of direction (dn,du)
if nn * (-1) + nu_ * (-1) < 0: nn, nu_ = -nn, -nu_   # point towards the inside (towards the origin)
a0 = (REAR_N + nn * WALL, REAR_FLAT_U + nu_ * WALL)   # point on inner line
def inner_at_n(n):  # u on inner slope for given n
    t = (n - a0[0]) / dn
    return a0[1] + t * du
N_IN = REAR_N - WALL
U_CORNER = inner_at_n(N_IN)
U_TOP_IN = inner_at_n(0.0)

def run(_context: str):
    app = adsk.core.Application.get()
    app.documents.add(adsk.core.DocumentTypes.FusionDesignDocumentType)
    design = adsk.fusion.Design.cast(app.activeProduct)
    design.designType = adsk.fusion.DesignTypes.ParametricDesignType
    root = design.rootComponent
    ext = root.features.extrudeFeatures
    VI = adsk.core.ValueInput.createByReal

    def poly(pts, name, plane=None):
        sk = root.sketches.add(plane or root.xYConstructionPlane)
        sk.name = name
        P = [adsk.core.Point3D.create(x / 10, y / 10, 0) for x, y in pts]
        Ls = sk.sketchCurves.sketchLines; lines = []
        for i in range(len(P)):
            a = lines[-1].endSketchPoint if lines else P[0]
            b = lines[0].startSketchPoint if i == len(P) - 1 else P[i + 1]
            lines.append(Ls.addByTwoPoints(a, b))
        return sk

    outer = [at_y(-CLOCK_T, 0), g(-CLOCK_T, 0), g(0, 0),
             g(0, TOP_U), g(REAR_N, REAR_FLAT_U), at_y(REAR_N, 0)]
    sk = poly(outer, 'v4_outer')
    ei = ext.createInput(sk.profiles.item(0), adsk.fusion.FeatureOperations.NewBodyFeatureOperation)
    ei.setSymmetricExtent(VI(2 * (Z_IN + END_T) / 10), True)
    body = ext.add(ei).bodies.item(0); body.name = 'Stand_v4'

    B = g(0, 0)
    inner = [B, (B[0], FLOOR_Y), at_y(N_IN, FLOOR_Y), g(N_IN, U_CORNER), g(0, U_TOP_IN)]
    sk = poly(inner, 'v4_cavity')
    ei = ext.createInput(sk.profiles.item(0), adsk.fusion.FeatureOperations.CutFeatureOperation)
    ei.setSymmetricExtent(VI(2 * Z_IN / 10), True)
    ei.participantBodies = [body]
    ext.add(ei)

    blk = [B, (B[0], FLOOR_Y), at_y(N_IN, FLOOR_Y), g(N_IN, BLOCK_U), g(0, BLOCK_U)]
    for sgn in (1, -1):
        pi = root.constructionPlanes.createInput()
        pi.setByOffset(root.xYConstructionPlane, VI(sgn * BLOCK_Z0 / 10))
        pl = root.constructionPlanes.add(pi)
        sk = poly(blk, 'v4_block_' + ('p' if sgn > 0 else 'm'), pl)
        ei = ext.createInput(sk.profiles.item(0), adsk.fusion.FeatureOperations.JoinFeatureOperation)
        ei.setOneSideExtent(adsk.fusion.DistanceExtentDefinition.create(VI((Z_IN - BLOCK_Z0 + 0.5) / 10)),
                            adsk.fusion.ExtentDirections.PositiveExtentDirection if sgn > 0 else adsk.fusion.ExtentDirections.NegativeExtentDirection)
        ei.participantBodies = [body]
        ext.add(ei)

    # cutters: screw holes + counterbores, USB cable hole
    tb = adsk.fusion.TemporaryBRepManager.get()
    EN = adsk.core.Vector3D.create(c, -s, 0); EU = adsk.core.Vector3D.create(s, c, 0)
    def P3(n, u, z):
        x, y = g(n, u); return adsk.core.Point3D.create(x / 10, y / 10, z / 10)
    def cyl(n0, n1, u, z, d): return tb.createCylinderOrCone(P3(n0, u, z), d / 20, P3(n1, u, z), d / 20)
    def box(n0, n1, u0, u1, z0, z1):
        return tb.createBox(adsk.core.OrientedBoundingBox3D.create(P3((n0 + n1) / 2, (u0 + u1) / 2, (z0 + z1) / 2), EN, EU,
                            (n1 - n0) / 10, (u1 - u0) / 10, (z1 - z0) / 10))
    cut = None
    def add(b):
        nonlocal cut
        if cut is None: cut = b
        else: tb.booleanOperation(cut, b, adsk.fusion.BooleanTypes.UnionBooleanType)
    for zs in (HOLE_Z, -HOLE_Z):
        add(cyl(-1.0, REAR_N + 2, HOLE_U, zs, D_THRU))
        add(cyl(CLAMP, REAR_N + 2, HOLE_U, zs, D_CB))
    r = SOCK_WU / 2   # stadium, long side along n
    add(box(SOCK_N - SOCK_WN / 2 + r, SOCK_N + SOCK_WN / 2 - r, SOCK_U - r, SOCK_U + r, Z_IN - 1, Z_IN + END_T + 1))
    for dn_ in (-(SOCK_WN / 2 - r), SOCK_WN / 2 - r):
        add(tb.createCylinderOrCone(P3(SOCK_N + dn_, SOCK_U, Z_IN - 1), r / 10, P3(SOCK_N + dn_, SOCK_U, Z_IN + END_T + 1), r / 10))
    add(box(SOCK_N - POCKET_WN / 2, SOCK_N + POCKET_WN / 2, SOCK_U - POCKET_WU / 2, SOCK_U + POCKET_WU / 2, Z_IN - 0.1, Z_IN + END_T - PANEL_T))
    bf = root.features.baseFeatures.add(); bf.startEdit()
    root.bRepBodies.add(cut, bf); bf.finishEdit(); bf.name = 'v4_cutters'
    tc = adsk.core.ObjectCollection.create(); tc.add(bf.bodies.item(0))
    ci = root.features.combineFeatures.createInput(body, tc)
    ci.operation = adsk.fusion.FeatureOperations.CutFeatureOperation
    root.features.combineFeatures.add(ci)

    bb = body.boundingBox
    print('U_CORNER', round(U_CORNER, 2), 'U_TOP_IN', round(U_TOP_IN, 2), 'outer', [(round(x, 2), round(y, 2)) for x, y in outer])
    print('bbox', [round(v * 10, 2) for v in (bb.minPoint.x, bb.minPoint.y, bb.minPoint.z, bb.maxPoint.x, bb.maxPoint.y, bb.maxPoint.z)],
          'vol', round(body.volume, 2), 'bodies', root.bRepBodies.count)
