# v5 chamfers: like v4, but nothing on end-face edges that meet the clock
# (clock-back line n=0, seat u=0, front face n=-15 flush with the clock face),
# plus a large chamfer on the non-functional top inner corner of both screw blocks.
import adsk.core, adsk.fusion, math, os

SCRIPT_DIR = os.environ.get('WORTUHR_STAND_DIR', '.')
exec(open(os.path.join(SCRIPT_DIR, 'v4_build.py'), encoding='utf-8').read().split('\ndef run(')[0])

LONG_PTS = [(4.06, 0.0), (52.29, 0.0)]  # table edges: front, rear (mm)
CH = 0.1        # cm, 45° edge chamfer
CH_BLOCK = 0.6  # cm, screw-block corner

def local(p):  # model point (cm) -> (n, u) mm
    return nu(p.x * 10, p.y * 10)

def on_clock_line(nu_pt):
    n, u = nu_pt
    return abs(n) < 0.05 or abs(n + CLOCK_T) < 0.05 or (abs(u) < 0.05 and -CLOCK_T - 0.05 <= n <= 0.05)

def run(_context: str):
    root = adsk.fusion.Design.cast(adsk.core.Application.get().activeProduct).rootComponent
    body = root.bRepBodies.itemByName('Stand_v4')
    zmax = body.boundingBox.maxPoint.z
    edges = adsk.core.ObjectCollection.create()
    picked = set(); skipped = 0
    def take(e):
        if e.tempId not in picked:
            picked.add(e.tempId); edges.add(e)
    for e in body.edges:
        a, b = e.startVertex.geometry, e.endVertex.geometry
        if abs(a.x - b.x) < 1e-4 and abs(a.y - b.y) < 1e-4 and abs(a.z - b.z) > 15:
            for x, y in LONG_PTS:
                if abs(a.x * 10 - x) < 0.05 and abs(a.y * 10 - y) < 0.05:
                    take(e)
    for f in body.faces:
        if f.geometry.surfaceType != adsk.core.SurfaceTypes.PlaneSurfaceType: continue
        if abs(abs(f.geometry.normal.z) - 1) > 1e-6 or abs(abs(f.pointOnFace.z) - zmax) > 1e-4: continue
        for lp in f.loops:
            if not lp.isOuter: continue
            for e in lp.edges:
                if on_clock_line(local(e.startVertex.geometry)) and on_clock_line(local(e.endVertex.geometry)):
                    skipped += 1
                    continue
                take(e)
    ci = root.features.chamferFeatures.createInput2()
    ci.chamferEdgeSets.addEqualDistanceChamferEdgeSet(edges, adsk.core.ValueInput.createByReal(CH), True)
    root.features.chamferFeatures.add(ci).name = 'v5_chamfers_45'

    blk = adsk.core.ObjectCollection.create()
    for e in body.edges:
        a, b = e.startVertex.geometry, e.endVertex.geometry
        if abs(abs(a.z) - BLOCK_Z0 / 10) > 1e-4 or abs(abs(b.z) - BLOCK_Z0 / 10) > 1e-4: continue
        la, lb = local(a), local(b)
        if abs(la[1] - BLOCK_U) < 0.05 and abs(lb[1] - BLOCK_U) < 0.05 and abs(la[0] - lb[0]) > 20:
            blk.add(e)
    ci = root.features.chamferFeatures.createInput2()
    ci.chamferEdgeSets.addEqualDistanceChamferEdgeSet(blk, adsk.core.ValueInput.createByReal(CH_BLOCK), False)
    root.features.chamferFeatures.add(ci).name = 'v5_block_corner'
    print('45° edges', edges.count, 'skipped clock edges', skipped, 'block edges', blk.count, 'vol', round(body.volume, 2))
