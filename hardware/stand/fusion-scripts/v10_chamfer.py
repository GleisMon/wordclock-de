# v10 = v9 + 45 deg chamfers on the remaining sharp outer edges, including those that meet the clock
# (user 12.10: "edges too sharp"): top knife edge of the rear wall, front/seat edge, end-face edges.
import adsk.core, adsk.fusion, math, os
exec(open(os.path.join(os.path.dirname(r'D:\___Claude\_Pub\Wortuhr\entwicklung\fusion_v10\x'), 'edges_dump.py'), encoding='utf-8').read().split('\ndef run(')[0])
CH = 0.1  # cm

def on_clock_line(p):
    n, u = p
    return abs(n) < 0.05 or abs(n + CLOCK_T) < 0.05 or (abs(u) < 0.05 and -CLOCK_T - 0.05 <= n <= 0.05)

def run(_context: str):
    root = adsk.fusion.Design.cast(adsk.core.Application.get().activeProduct).rootComponent
    body = [b for b in root.bRepBodies if b.name.startswith('Stand')][0]
    v0 = body.volume
    edges = adsk.core.ObjectCollection.create()
    for e in body.edges:
        if e.geometry.objectType != adsk.core.Line3D.classType(): continue
        ci = convex_info(e)
        if not ci or not ci[0] or ci[1] < 85: continue
        a, b = e.startVertex.geometry, e.endVertex.geometry
        if min(abs(a.z), abs(b.z)) * 10 < 114: continue
        if on_clock_line(nu(a.x*10, a.y*10)) and on_clock_line(nu(b.x*10, b.y*10)):
            edges.add(e)
    ci = root.features.chamferFeatures.createInput2()
    ci.chamferEdgeSets.addEqualDistanceChamferEdgeSet(edges, adsk.core.ValueInput.createByReal(CH), False)
    root.features.chamferFeatures.add(ci).name = 'v10_chamfers_clock_edges'
    body.name = 'Stand_v10'
    print('edges', edges.count, 'vol', round(v0, 3), '->', round(body.volume, 3), 'bodies', root.bRepBodies.count)
