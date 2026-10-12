import adsk.core, adsk.fusion, math, os
SCRIPT_DIR = r'D:\___Claude\_Pub\Wortuhr\wordclock-de\hardware\stand\fusion-scripts'
os.environ['WORTUHR_STAND_DIR'] = SCRIPT_DIR
exec(open(os.path.join(SCRIPT_DIR, 'v4_build.py'), encoding='utf-8').read().split('\ndef run(')[0])

def convex_info(e):
    if e.faces.count != 2: return None
    f1, f2 = e.faces.item(0), e.faces.item(1)
    ev = e.evaluator
    _, t0, t1 = ev.getParameterExtents()
    _, p = ev.getPointAtParameter((t0 + t1) / 2)
    _, tan = ev.getTangent((t0 + t1) / 2)
    _, n1 = f1.evaluator.getNormalAtPoint(p)
    _, n2 = f2.evaluator.getNormalAtPoint(p)
    co = None
    for c in e.coEdges:
        if c.loop.face == f1: co = c
    if co is None: return None
    if co.isOpposedToEdge: tan.scaleBy(-1)
    cr = n1.crossProduct(n2)
    ang = math.degrees(math.acos(max(-1, min(1, n1.dotProduct(n2)))))
    return cr.dotProduct(tan) > 0, ang, p

def run(_context: str):
    root = adsk.fusion.Design.cast(adsk.core.Application.get().activeProduct).rootComponent
    out = []
    for b in root.bRepBodies:
        out.append('BODY %s vol %.2f faces %d edges %d' % (b.name, b.volume, b.faces.count, b.edges.count))
    body = [b for b in root.bRepBodies if b.name.startswith('Stand')][0]
    bb = body.boundingBox
    out.append('bbox %s %s' % ([round(v*10,2) for v in bb.minPoint.asArray()], [round(v*10,2) for v in bb.maxPoint.asArray()]))
    tl = adsk.fusion.Design.cast(adsk.core.Application.get().activeProduct).timeline
    out.append('timeline: ' + ', '.join(tl.item(i).name for i in range(tl.count)))
    rows = []
    for e in body.edges:
        if e.length < 0.1: continue
        ci = convex_info(e)
        if not ci or not ci[0] or ci[1] < 20: continue
        a, b2 = e.startVertex.geometry, e.endVertex.geometry
        na, nb = nu(a.x*10, a.y*10), nu(b2.x*10, b2.y*10)
        rows.append('ang %5.1f len %6.1f  (n %6.2f u %6.2f z %7.2f) -> (n %6.2f u %6.2f z %7.2f) %s' % (ci[1], e.length*10, na[0], na[1], a.z*10, nb[0], nb[1], b2.z*10, e.geometry.objectType.split('::')[-1]))
    rows.sort()
    open(r'D:\___Claude\_Pub\Wortuhr\entwicklung\fusion_v10\edges.txt', 'w', encoding='utf-8').write('\n'.join(out + rows))
    print('\n'.join(out)); print(len(rows), 'convex edges')
