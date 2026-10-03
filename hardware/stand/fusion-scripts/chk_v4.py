# v2 fit check with ghosts (read-only)
import adsk.core, adsk.fusion, math, os
SCRIPT_DIR = os.environ.get('WORTUHR_STAND_DIR', '.')  # folder with these scripts
_src = open(os.path.join(SCRIPT_DIR, 'v4_build.py'), encoding='utf-8').read().split('def run')[0]
exec(_src)

def run(_context: str):
    root = adsk.fusion.Design.cast(adsk.core.Application.get().activeProduct).rootComponent
    body = root.bRepBodies.itemByName('Stand_v4')
    tb = adsk.fusion.TemporaryBRepManager.get()
    EN = adsk.core.Vector3D.create(c, -s, 0); EU = adsk.core.Vector3D.create(s, c, 0)
    def P3(n, u, z):
        x, y = g(n, u); return adsk.core.Point3D.create(x / 10, y / 10, z / 10)
    def cyl(n0, n1, u, z, d): return tb.createCylinderOrCone(P3(n0, u, z), d / 20, P3(n1, u, z), d / 20)
    def box(n0, n1, u0, u1, z0, z1):
        return tb.createBox(adsk.core.OrientedBoundingBox3D.create(P3((n0 + n1) / 2, (u0 + u1) / 2, (z0 + z1) / 2), EN, EU,
                            (n1 - n0) / 10, (u1 - u0) / 10, (z1 - z0) / 10))
    def inter(gh):
        a = tb.copy(body)
        tb.booleanOperation(a, gh, adsk.fusion.BooleanTypes.IntersectionBooleanType)
        return 0.0 if a.faces.count == 0 else round(a.volume * 1000, 2)
    e = 0.02
    def stad(L, W, z0, z1):
        r = W / 2
        b = box(SOCK_N - L / 2 + r, SOCK_N + L / 2 - r, SOCK_U - r, SOCK_U + r, z0, z1)
        for d in (-(L / 2 - r), L / 2 - r):
            tb.booleanOperation(b, tb.createCylinderOrCone(P3(SOCK_N + d, SOCK_U, z0), r / 10, P3(SOCK_N + d, SOCK_U, z1), r / 10), adsk.fusion.BooleanTypes.UnionBooleanType)
        return b
    ck = {
        'clock (want 0)': box(-CLOCK_T + e, -e, e, CLOCK_H, -CLOCK_W / 2, CLOCK_W / 2),
        'screw R (0)': cyl(0, CLAMP, HOLE_U, HOLE_Z, 3.7),
        'screw L (0)': cyl(0, CLAMP, HOLE_U, -HOLE_Z, 3.7),
        'head R (0)': cyl(CLAMP + e, CLAMP + 2, HOLE_U, HOLE_Z, 5.9),
        'head L (0)': cyl(CLAMP + e, CLAMP + 2, HOLE_U, -HOLE_Z, 5.9),
        'ring under head (>0)': cyl(CLAMP - 0.5, CLAMP - e, HOLE_U, HOLE_Z, 5.9),
        'top contact strip (>0)': box(0, 0.3, U_TOP_IN, TOP_U, -90, 90),
        'bottom contact blocks (>0)': box(0, 0.3, 0, BLOCK_U, BLOCK_Z0 + 2, Z_IN - 1),
        'LED wire R (0)': box(e, 15, 16.6, 29.8, CLOCK_W / 2 - 9.2, CLOCK_W / 2 - 5.5),
        'LED wire L (0)': box(e, 15, 16.6, 29.8, -CLOCK_W / 2 + 5.5, -CLOCK_W / 2 + 9.2),
        'NodeMCU upright 58x31x10 (0)': box(0.5, 10.5, 0.2, 31.2, -29, 29),
        'NodeMCU upright 60x33x12 (0)': box(0.3, 12.3, 0.1, 33.1, -30, 30),
        'NodeMCU upright 58x31x14 (0)': box(0.5, 14.5, 0.2, 31.2, -29, 29),
        'socket body 13.7x5.1, 15 deep (0)': box(SOCK_N - 6.85, SOCK_N + 6.85, SOCK_U - 2.55, SOCK_U + 2.55, Z_IN + END_T - 15, Z_IN + END_T + 0.5),
        'clip room 19x7 behind 1.5 panel (0)': box(SOCK_N - 9.5, SOCK_N + 9.5, SOCK_U - 3.5, SOCK_U + 3.5, Z_IN - 6, Z_IN + END_T - PANEL_T - 0.02),
        'panel 1.5 around hole (>0)': box(SOCK_N - 10, SOCK_N + 10, SOCK_U - 3.5, SOCK_U + 3.5, Z_IN + END_T - PANEL_T + 0.05, Z_IN + END_T - 0.05),
        'LED wire corridor n 0..8 at the socket end (0)': box(0.05, 8.0, 16.6, 29.8, CLOCK_W / 2 - 9.2, Z_IN - 0.05),
        'screw block solid under the socket (>0, ~ 21*2*15)': box(SOCK_N - 10.5, SOCK_N + 10.5, BLOCK_U - 2, BLOCK_U - 0.05, Z_IN - 15, Z_IN - 0.05),
        'socket flange 16x9 outside (0)': box(SOCK_N - 8, SOCK_N + 8, SOCK_U - 4.5, SOCK_U + 4.5, Z_IN + END_T + 0.02, Z_IN + END_T + 2),
        'flange seat on end face (>0)': box(SOCK_N - 8, SOCK_N + 8, SOCK_U - 4.5, SOCK_U + 4.5, Z_IN + END_T - 0.3, Z_IN + END_T - 0.02),
    }
    for k, gh in ck.items():
        print(k, inter(gh))
