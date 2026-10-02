# v5 fit check (read-only): OLED module and window + the v4 ghosts
import adsk.core, adsk.fusion, math, os
SCRIPT_DIR = os.environ.get('WORTUHR_STAND_DIR', '.')
exec(open(os.path.join(SCRIPT_DIR, 'v5_screen.py'), encoding='utf-8').read().split('\ndef run(')[0])

def run(_context: str):
    root = adsk.fusion.Design.cast(adsk.core.Application.get().activeProduct).rootComponent
    body = root.bRepBodies.itemByName('Stand_v5')
    tb = adsk.fusion.TemporaryBRepManager.get()
    D = adsk.core.Vector3D.create(c * _d[0] + s * _d[1], -s * _d[0] + c * _d[1], 0)
    O = adsk.core.Vector3D.create(c * _o[0] + s * _o[1], -s * _o[0] + c * _o[1], 0)
    def box(t0, t1, s0, s1, z0, z1):
        return tb.createBox(adsk.core.OrientedBoundingBox3D.create(P((t0 + t1) / 2, (s0 + s1) / 2, (z0 + z1) / 2), D, O,
                            (t1 - t0) / 10, (s1 - s0) / 10, (z1 - z0) / 10))
    def inter(gh):
        a = tb.copy(body)
        tb.booleanOperation(a, gh, adsk.fusion.BooleanTypes.IntersectionBooleanType)
        return 0.0 if a.faces.count == 0 else round(a.volume * 1000, 2)
    pt0, pt1 = T_CENTER - POCKET_W / 2, T_CENTER + POCKET_W / 2
    floor_s = -PAD_T + POCKET_D
    e = 0.02
    print('OLED module 38.5x12x2.5 in pocket (0):', inter(box(pt0 + 0.1, pt1 - 0.1, floor_s - 2.5, floor_s - e, -19.25, 19.25)))
    print('module backside + wires 38x12x6 inside (0):', inter(box(pt0 + 0.1, pt1 - 0.1, floor_s - 2.5 - 6, floor_s - 2.5, -19, 19)))
    print('view through window 23x7.6 (0):', inter(box(pt0 + WIN_LO + 0.3, pt1 - WIN_HI - 0.3, floor_s - 0.5, 8, -11.5, 11.5)))
    print('skin over module (>0, ~(39*12.2-24*8.2)*0.78):', inter(box(pt0, pt1, floor_s + e, WALL - e, -19.5, 19.5)))
    # measure skin thickness along O at the pocket centre-edge
    lo, hi = floor_s, floor_s + 3
    pt = (pt0 + 1.0, 0, 15.0)
    for _ in range(30):
        m = (lo + hi) / 2
        q = P(pt[0], m, pt[2])
        if body.pointContainment(q) == adsk.fusion.PointContainment.PointInsidePointContainment: lo = m
        else: hi = m
    print('skin thickness mm:', round(lo - floor_s, 2))
    ns = {}
    src = open(os.path.join(SCRIPT_DIR, 'chk_v4.py'), encoding='utf-8').read().replace("'Stand_v4'", "'Stand_v5'")
    exec(src, ns)
    ns['run'](_context)
