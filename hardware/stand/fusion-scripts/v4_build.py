# WordClockStand v4 — v3 + rounded (stadium) USB-C socket cutout, end wall thinned to 1.5 mm around it
import adsk.core, adsk.fusion, math

TILT = 10.0
# Real clock (60 LED/m version, re-measured 03.10 with a precise rule): 230 wide, 17 thick incl. back plate,
# holes 212 apart, 9 mm above the bottom edge. (The 74 LED/m 3MF is 187.12 x 15, holes 173.12 / 7.)
CLOCK_T, CLOCK_W, CLR_END = 17.0, 230.0, 0.5
CLOCK_H = 230.0                 # only for the fit check (not measured, roughly square)
LIP_T, LIP_H = 4.0, 7.0
CLAMP, CB_DEPTH = 30.0, 2.5
REAR_N = CLAMP + CB_DEPTH       # 32.5: flat rear face for the screw heads
REAR_FLAT_U = 16.0              # flat rear face up to here (counterbore reaches u=12.3)
TOP_U = 52.0                    # rear wall meets the clock back here
WALL = 2.5
FLOOR_Y = 3.0
END_T = 3.0
HOLE_U, HOLE_Z = 9.0, 106.0      # hole centres: 9 mm above the clock's bottom edge, 212 apart
BLOCK_Z0, BLOCK_U = HOLE_Z - 8.5, 15.0  # solid screw blocks at the ends (counterbore top u=12.3)
D_THRU, D_CB = 4.2, 6.6
# USB-C panel socket (body 13.7 x 5.1): rectangular cutout, height +0.7 for FDM (5.3 stadium did not fit, 03.10);
# moved down/back, away from the LED wires leaving the clock at n=0, u>=16.6
SOCK_N, SOCK_U, SOCK_WN, SOCK_WU = 16.0, 16.5, 13.9, 5.8
BAY_U0 = 13.0                   # recess in the screw block for socket body + clips (screw hole top at u=11.1)
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
    # floor -> clock seat edge: R5 arc tangent to the floor instead of a 2 mm vertical step (printing, 03.10)
    R_STEP = 5.0
    hgt = B[1] - FLOOR_Y
    cx = B[0] + math.sqrt(R_STEP ** 2 - (R_STEP - hgt) ** 2)
    a0 = math.atan2(B[1] - (FLOOR_Y + R_STEP), B[0] - cx)
    arc = [(cx + R_STEP * math.cos(a0 + (-math.pi / 2 - a0) * k / 8), FLOOR_Y + R_STEP + R_STEP * math.sin(a0 + (-math.pi / 2 - a0) * k / 8)) for k in range(1, 9)]
    inner = [B] + arc + [at_y(N_IN, FLOOR_Y), g(N_IN, U_CORNER), g(0, U_TOP_IN)]
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
    add(box(SOCK_N - SOCK_WN / 2, SOCK_N + SOCK_WN / 2, SOCK_U - SOCK_WU / 2, SOCK_U + SOCK_WU / 2, Z_IN - 1, Z_IN + END_T + 1))
    add(box(SOCK_N - 10.5, SOCK_N + 10.5, BAY_U0, BLOCK_U + 1, Z_IN - 17, Z_IN + 0.05))  # bay in the +z screw block
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
