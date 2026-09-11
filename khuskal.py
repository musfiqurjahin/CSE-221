def kruskal(e, n):
    p = list(range(n))
    for w,u,v in sorted(e):
        while p[u] != u: u = p[u]
        while p[v] != v: v = p[v]
        if u != v:
            print(w,u,v)
            p[u] = v

kruskal([(2,0,1),(3,0,2),(1,1,2),(4,1,3),(5,2,3)],4)