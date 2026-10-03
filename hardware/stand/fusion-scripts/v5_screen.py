# WordClockStand v5 — v4 + window for a 0.91" SSD1306 128x32 I2C module in the sloped rear wall.
# Pocket/window geometry taken from the Reflex MainBox lid: module pocket 39 x 12.2 x 2.5,
# window 24 x 8.2 (7.5 mm from both short sides, 2.5 / 1.5 mm from the long sides), 0.5 chamfer.
# The rear wall is only 2.5 mm, so a 0.8 mm pad is added inside; skin over the module 0.8 mm.
import adsk.core, adsk.fusion, math, os

SCRIPT_DIR = os.environ.get('WORTUHR_STAND_DIR', '.')
exec(open(os.path.join(SCRIPT_DIR, 'v4_build.py'), encoding='utf-8').read().split('def run')[0])

SKIN = 0.8                       # wall left over the module
POCKET_L, POCKET_W, POCKET_D = 39.0, 12.2, WALL - SKIN   # 1.7 deep: wall stays 2.5 everywhere, no frame
WIN_L, WIN_W = 24.0, 8.2
WIN_LO, WIN_HI = 2.5, 1.5        # window margins: down-slope side / up-slope side
PAD_T = 0.0                      # no extra pad (user: wall flush, module back may stick out ~0.8 mm inside)
PAD_MARGIN = 3.0                 # pad overlap around the pocket
RIM_H, RIM_W = 2.0, 1.6          # inner rim around the pocket, 2 mm above the inner wall face (fit test 03.10:
                                 # the 1.7 mm pocket held too little); outer flank 45 deg into the wall
T_CENTER = 27.0                  # pocket centre, mm along the inner slope from its lower corner
Z_CENTER = 0.0
WIN_CHAMFER = 0.05               # cm

# inner slope line (n, u): from (N_IN, U_CORNER) up to (0, U_TOP_IN)
_a = (N_IN, U_CORNER); _b = (0.0, U_TOP_IN)
_len = math.hypot(_b[0] - _a[0], _b[1] - _a[1])
_d = ((_b[0] - _a[0]) / _len, (_b[1] - _a[1]) / _len)      # up the slope
_o = (_d[1], -_d[0])                                        # outward (into the wall, away from the cavity)
if _o[0] < 0: _o = (-_o[0], -_o[1])                         # outward has +n (towards the rear)

def P(t, s, z):
    """t along the slope from the lower inner corner, s outward from the inner face, z along the stand."""
    n = _a[0] + _d[0] * t + _o[0] * s
    u = _a[1] + _d[1] * t + _o[1] * s
    x, y = g(n, u)
    return adsk.core.Point3D.create(x / 10, y / 10, z / 10)

def run(_context: str):
    app = adsk.core.Application.get()
    root = adsk.fusion.Design.cast(app.activeProduct).rootComponent
    body = root.bRepBodies.itemByName('Stand_v4')
    tb = adsk.fusion.TemporaryBRepManager.get()
    D = adsk.core.Vector3D.create(*(v for v in (c * _d[0] + s * _d[1], -s * _d[0] + c * _d[1], 0)))
    O = adsk.core.Vector3D.create(c * _o[0] + s * _o[1], -s * _o[0] + c * _o[1], 0)
    D.normalize(); O.normalize()

    def box(t0, t1, s0, s1, z0, z1):
        ctr = P((t0 + t1) / 2, (s0 + s1) / 2, (z0 + z1) / 2)
        return tb.createBox(adsk.core.OrientedBoundingBox3D.create(ctr, D, O, (t1 - t0) / 10, (s1 - s0) / 10, (z1 - z0) / 10))

    pt0, pt1 = T_CENTER - POCKET_W / 2, T_CENTER + POCKET_W / 2
    pz0, pz1 = Z_CENTER - POCKET_L / 2, Z_CENTER + POCKET_L / 2
    pad = box(pt0 - PAD_MARGIN, pt1 + PAD_MARGIN, -PAD_T, 0.5, pz0 - PAD_MARGIN, pz1 + PAD_MARGIN)
    floor_s = -PAD_T + POCKET_D                                   # pocket floor, measured from the old inner face
    if RIM_H > 0:  # frustum on the inner slope face (sketch + extrude with 45 deg taper), hollowed by the pocket below
        pc = P(T_CENTER, 0, Z_CENTER)
        face = None
        for f in body.faces:
            if f.geometry.surfaceType != adsk.core.SurfaceTypes.PlaneSurfaceType: continue
            nrm = f.evaluator.getNormalAtPoint(f.pointOnFace)[1]
            if nrm.dotProduct(O) < -0.99 and abs(f.pointOnFace.vectorTo(pc).dotProduct(O)) < 1e-4:
                face = f
        sk = root.sketches.add(face)
        sk.name = 'v5_oled_rim'
        def sp(t, z):
            q = sk.modelToSketchSpace(P(t, 0, z)); q.z = 0
            return q
        w = RIM_W + RIM_H
        sk.sketchCurves.sketchLines.addTwoPointRectangle(sp(pt0 - w, pz0 - w), sp(pt1 + w, pz1 + w))
        want = (POCKET_W + 2 * w) * (POCKET_L + 2 * w) / 100   # cm2; the face's own loops make extra profiles
        prof = min((sk.profiles.item(i) for i in range(sk.profiles.count)), key=lambda p: abs(p.areaProperties().area - want))
        ext = root.features.extrudeFeatures
        v0 = body.volume
        added = 0
        EDir = adsk.fusion.ExtentDirections
        for dirn, taper in ((EDir.PositiveExtentDirection, '-45 deg'), (EDir.PositiveExtentDirection, '45 deg'),
                            (EDir.NegativeExtentDirection, '-45 deg'), (EDir.NegativeExtentDirection, '45 deg')):
            ei = ext.createInput(prof, adsk.fusion.FeatureOperations.JoinFeatureOperation)
            ei.setOneSideExtent(adsk.fusion.DistanceExtentDefinition.create(adsk.core.ValueInput.createByReal(RIM_H / 10)),
                                dirn, adsk.core.ValueInput.createByString(taper))
            ei.participantBodies = [body]
            try:
                feat = ext.add(ei)
            except RuntimeError:
                continue
            added = (body.volume - v0) * 1000
            if 1400 < added < 1700:   # shrinking frustum (into the cavity) ~1.54 cm3
                feat.name = 'v5_oled_rim'
                break
            feat.deleteMe()
        print('rim added mm3', round(added))
    pocket = box(pt0, pt1, -max(PAD_T, RIM_H) - 1.0, floor_s, pz0, pz1)
    win = box(pt0 + WIN_LO, pt1 - WIN_HI, floor_s - 0.1, WALL + 2.0, Z_CENTER - WIN_L / 2, Z_CENTER + WIN_L / 2)
    tb.booleanOperation(pocket, win, adsk.fusion.BooleanTypes.UnionBooleanType)

    bf = root.features.baseFeatures.add(); bf.startEdit()
    if PAD_T > 0: root.bRepBodies.add(pad, bf)
    root.bRepBodies.add(pocket, bf)
    bf.finishEdit(); bf.name = 'v5_oled'
    ops = []
    if PAD_T > 0: ops.append((bf.bodies.item(0), adsk.fusion.FeatureOperations.JoinFeatureOperation))
    ops.append((bf.bodies.item(bf.bodies.count - 1), adsk.fusion.FeatureOperations.CutFeatureOperation))
    for tool, op in ops:
        tc = adsk.core.ObjectCollection.create(); tc.add(tool)
        ci = root.features.combineFeatures.createInput(body, tc); ci.operation = op
        root.features.combineFeatures.add(ci)
    body.name = 'Stand_v5'

    # chamfer the window edges on the outer face
    edges = adsk.core.ObjectCollection.create()
    for f in body.faces:
        if f.geometry.surfaceType != adsk.core.SurfaceTypes.PlaneSurfaceType: continue
        n = f.geometry.normal
        if f.evaluator.getNormalAtPoint(f.pointOnFace)[1].dotProduct(O) < 0.99: continue
        for lp in f.loops:
            if not lp.isOuter:
                for e in lp.edges: edges.add(e)
    ci = root.features.chamferFeatures.createInput2()
    ci.chamferEdgeSets.addEqualDistanceChamferEdgeSet(edges, adsk.core.ValueInput.createByReal(WIN_CHAMFER), True)
    root.features.chamferFeatures.add(ci).name = 'v5_window_chamfer'
    print('slope dir', [round(v, 3) for v in (D.x, D.y)], 'out', [round(v, 3) for v in (O.x, O.y)],
          'inner slope len', round(_len, 1), 'window edges', edges.count, 'vol', round(body.volume, 2))
