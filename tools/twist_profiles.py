"""Developer script: measure how the official figures share limb weights with their helper bones (src/core/twist.py).

Usage: python tools/twist_profiles.py  (needs src/assets/bases); prints the median share of each bone along each limb."""
import sys, json; sys.path.insert(0, str(__import__('pathlib').Path(__file__).resolve().parents[1]))
import numpy as np
from src.core.roster import find_entry
from src.core.base_character import BaseCharacter, S1_STRIDE
idx_json=json.load(open('src/assets/bases/index.json'))
GROUPS={'upper_arm':(('Shoulder','Shoulder_Spin','Shoulder_OL'),'Shoulder','Elbow'),
        'forearm':(('Elbow','Forearm_OL','Elbow_OL'),'Elbow','Hand'),
        'thigh':(('Thigh','Thigh_OL'),'Thigh','Knee')}
bins=np.linspace(-0.2,1.0,13); res={g:[] for g in GROUPS}
for actor,row in idx_json.items():
    if row['status']['state']!='standard': continue
    b=BaseCharacter.from_pack(find_entry(actor)); prim=b.body_primitive; n=prim.vertex_count
    s1=np.frombuffer(b._vbuf[prim.s1_offset:prim.s1_offset+n*S1_STRIDE],np.uint8).reshape(n,S1_STRIDE)
    idx,w=s1[:,0:4].astype(int),s1[:,4:8]/255.0
    verts=b.body_mesh[0]; names=list(b.bone_names); J=b.joints
    W=np.zeros((n,len(names)))
    for k in range(4): np.add.at(W,(np.arange(n),idx[:,k]),w[:,k])
    for side,sgn in (('Left',1),('Right',-1)):
        for g,(bones,a,e) in GROUPS.items():
            cols=[names.index(f'{x}_{side}') if f'{x}_{side}' in names else None for x in bones]
            if cols[0] is None or cols[1] is None: continue
            G=np.stack([W[:,c] if c is not None else np.zeros(n) for c in cols],1)
            tot=G.sum(1); A,E=J[f'{a}_{side}'],J[f'{e}_{side}']
            t=((verts-A)@(E-A))/np.dot(E-A,E-A); m=tot>0.3
            prof=np.full((len(bins)-1,len(bones)),np.nan)
            for i,(lo,hi) in enumerate(zip(bins[:-1],bins[1:])):
                s=m&(t>=lo)&(t<hi)
                if s.sum()>3: prof[i]=(G[s]/tot[s,None]).mean(0)
            res[g].append(prof)
out={'t':list(np.round((bins[:-1]+bins[1:])/2,3))}
for g,p in res.items():
    med=np.nanmedian(np.array(p),0); med=np.nan_to_num(med); 
    out[g]={'bones':GROUPS[g][0],'shares':np.round(med,3).tolist(),'n':len(p)}
    print(g,len(p)); print(np.round(med,2).T)
json.dump(out, open('build/twist_profiles.json', 'w'))
