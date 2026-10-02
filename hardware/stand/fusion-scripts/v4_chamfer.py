# step 3: 45° chamfers (not on clock-contact edges)
import adsk.core, adsk.fusion

LONG_PTS = [(4.06, 0.0), (52.29, 0.0)]  # bed-front, bed-rear (mm)
CH = 0.1  # cm

def run(_context: str):
    app = adsk.core.Application.get()
    root = adsk.fusion.Design.cast(app.activeProduct).rootComponent
    body = root.bRepBodies.itemByName('Stand_v4')
    zmax = body.boundingBox.maxPoint.z
    edges = adsk.core.ObjectCollection.create()
    picked = set()
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
        n = f.geometry.normal if f.geometry.surfaceType == adsk.core.SurfaceTypes.PlaneSurfaceType else None
        if n and abs(abs(n.z) - 1) < 1e-6 and abs(abs(f.pointOnFace.z) - zmax) < 1e-4:
            for e in f.loops.item(0).edges if f.loops.item(0).isOuter else []:
                take(e)
            for lp in f.loops:
                if lp.isOuter:
                    for e in lp.edges: take(e)
    print('edges', edges.count)
    ci = root.features.chamferFeatures.createInput2()
    ci.chamferEdgeSets.addEqualDistanceChamferEdgeSet(edges, adsk.core.ValueInput.createByReal(CH), True)
    ch = root.features.chamferFeatures.add(ci)
    ch.name = 'v4_chamfers_45'
    print('ok vol', round(body.volume, 2), 'faces', body.faces.count)
