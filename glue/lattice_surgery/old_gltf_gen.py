import json

# constant sqrt(2)/2
SQ2 = 0.70710678118
OLSCO_FILE_NAME = 'sample.json'
BASE_GLTF = 'base.gltf'
SEP = 1.1

with open(BASE_GLTF, 'r') as f:
    gltf = json.load(f)

with open(OLSCO_FILE_NAME, 'r') as f:
    olsco = json.load(f)

X_BOUND = olsco['X_BOUND']
Y_BOUND = olsco['Y_BOUND']
Z_BOUND = olsco['Z_BOUND']
NodeY = olsco['NodeY']
ExistX = olsco['ExistX']
ColorX = olsco['ColorX']
ExistY = olsco['ExistY']
ColorY = olsco['ColorY']
ExistZ = olsco['ExistZ']
ColorZ = olsco['ColorZ']


gltf['nodes'] = [{"name":"smaple"},]

# the edges
for x in range(X_BOUND):
    for y in range(Y_BOUND):
        for z in range(Z_BOUND):
            if ExistX[x][y][z]:
                if ColorX[x][y][z]:
                    gltf['nodes'].append({
                        "name":f"edgeX({x},{y},{z})-Color1",
                        "mesh": 0,
                        "rotation": [0, 0, SQ2, SQ2],
                        "translation": [SEP*x+SEP, SEP*z, -SEP*y] })
                else:
                    gltf['nodes'].append({
                        "name":f"edgeX({x},{y},{z})-Color0",
                        "mesh": 0,
                        "rotation": [0.5, 0.5, -0.5, 0.5],
                        "translation": [SEP*x+1, SEP*z, -SEP*y] })
            if ExistY[x][y][z]:
                if ColorY[x][y][z]:
                    gltf['nodes'].append({
                        "name":f"edgeY({x},{y},{z})-Color1",
                        "mesh": 0,
                        "rotation": [SQ2, 0, 0, SQ2],
                        "translation": [SEP*x, SEP*z, -SEP*y-SEP] })
                else:
                    gltf['nodes'].append({
                        "name":f"edgeX({x},{y},{z})-Color0",
                        "mesh": 0,
                        "rotation": [-0.5, 0.5, -0.5, 0.5],
                        "translation": [SEP*x+1, SEP*z+1, -SEP*y-1] })
            if ExistZ[x][y][z]:
                gltf['nodes'].append({
                    "name":f"edgeY({x},{y},{z})-Color1",
                    "mesh": 1,
                    "translation": [SEP*x, SEP*z+1, -SEP*y] })
                
# the vertices
for x in range(X_BOUND):
    for y in range(Y_BOUND):
        for z in range(Z_BOUND):
            exists = {'-x':0, '+x':0, '-y':0, '+y':0, '-z':0, '+z':0}
            if x>0 and ExistX[x-1][y][z]:
                exists['-x'] = 1
            if ExistX[x][y][z]:
                exists['+x'] = 1
            if y>0 and ExistY[x][y-1][z]:
                exists['-y'] = 1
            if ExistY[x][y][z]:
                exists['+y'] = 1
            if z>0 and ExistZ[x][y][z-1]:
                exists['-z'] = 1
            if ExistZ[x][y][z]:
                exists['+z'] = 1

            degree = sum([v for (k,v) in exists.items()])
            if degree == 1:
                if NodeY[x][y][z]:
                    if exists['+z']:
                        gltf['nodes'].append({
                            "name":f"InitY({x},{y},{z})",
                            "mesh": 4,
                            "translation": [SEP*x, SEP*z, -SEP*y] })
                    if exists['-z']:
                        gltf['nodes'].append({
                            "name":f"MeasY({x},{y},{z})",
                            "mesh": 4,
                            "rotation": [1, 0, 0, 0],
                            "translation": [SEP*x, SEP*z+1, -SEP*y-1] })
                else:
                    pass
            elif degree == 2:
                if NodeY[x][y][z]:
                    gltf['nodes'].append({
                        "name":f"MeasInitY({x},{y},{z})",
                        "mesh": 6,
                        "translation": [SEP*x, SEP*z, -SEP*y] })
                else:
                    if exists['+z'] and exists['-z']:
                        if ColorZ[x][y][z]:
                            gltf['nodes'].append({
                                "name":f"PassZ({x},{y},{z})-color1",
                                "mesh": 5,
                                "rotation": [0, SQ2, 0, SQ2],
                                "translation": [SEP*x+1, SEP*z, -SEP*y] })
                        else:
                            gltf['nodes'].append({
                                "name":f"PassZ({x},{y},{z})-color0",
                                "mesh": 5,
                                "translation": [SEP*x, SEP*z, -SEP*y] })
                    
                    if exists['+y'] and exists['-y']:
                        if ColorY[x][y][z]:
                            gltf['nodes'].append({
                                "name":f"PassY({x},{y},{z})",
                                "mesh": 5,
                                "rotation": [SQ2, 0, 0, SQ2],
                                "translation": [SEP*x, SEP*z, -SEP*y-1] })
                        else:
                            gltf['nodes'].append({
                                "name":f"PassY({x},{y},{z})",
                                "mesh": 5,
                                "rotation": [0.5, 0.5, 0.5, 0.5],
                                "translation": [SEP*x+1, SEP*z, -SEP*y-1] })
                            
                    if exists['+x'] and exists['-x']:
                        if ColorX[x][y][z]:
                            gltf['nodes'].append({
                                "name":f"PassX({x},{y},{z})",
                                "mesh": 5,
                                "rotation": [0, 0, SQ2, SQ2],
                                "translation": [SEP*x+1, SEP*z, -SEP*y] })
                        else:
                            gltf['nodes'].append({
                                "name":f"PassX({x},{y},{z})",
                                "mesh": 5,
                                "rotation": [0.5, -0.5, 0.5, 0.5],
                                "translation": [SEP*x+1, SEP*z, -SEP*y-1] })
                    
                    if exists['-x'] and exists['+y']:
                        if ColorX[x-1][y][z]:
                            gltf['nodes'].append({
                                "name":f"Turn-X&+Y({x},{y},{z})",
                                "mesh": 7,
                                "rotation": [-0.5, -0.5, 0.5, 0.5],
                                "translation": [SEP*x, SEP*z, -SEP*y] })
                        else:
                            gltf['nodes'].append({
                                "name":f"Turn-X&+Y({x},{y},{z})",
                                "mesh": 8,
                                "rotation": [-0.5, -0.5, 0.5, 0.5],
                                "translation": [SEP*x, SEP*z, -SEP*y] })
                    
                    if exists['+x'] and exists['-y']:
                        if ColorX[x][y][z]:
                            gltf['nodes'].append({
                                "name":f"Turn+X&-Y({x},{y},{z})",
                                "mesh": 7,
                                "rotation": [0, 0, -SQ2, SQ2],
                                "translation": [SEP*x, SEP*z+1, -SEP*y] })
                        else:
                            gltf['nodes'].append({
                                "name":f"Turn+X&-Y({x},{y},{z})",
                                "mesh": 8,
                                "rotation": [0, 0, -SQ2, SQ2],
                                "translation": [SEP*x, SEP*z+1, -SEP*y] })

                    if exists['+x'] and exists['+y']:
                        if ColorX[x][y][z]:
                            gltf['nodes'].append({
                                "name":f"Turn+X&+Y({x},{y},{z})",
                                "mesh": 7,
                                "rotation": [-0.5, 0.5, -0.5, 0.5],
                                "translation": [SEP*x, SEP*z+1, -SEP*y] })
                        else:
                            gltf['nodes'].append({
                                "name":f"Turn+X&+Y({x},{y},{z})",
                                "mesh": 8,
                                "rotation": [-0.5, 0.5, -0.5, 0.5],
                                "translation": [SEP*x, SEP*z+1, -SEP*y] })
                    
                    if exists['-x'] and exists['-y']:
                        if ColorX[x-1][y][z]:
                            gltf['nodes'].append({
                                "name":f"Turn-X&-Y({x},{y},{z})",
                                "mesh": 7,
                                "rotation": [0.5, -0.5, -0.5, 0.5],
                                "translation": [SEP*x, SEP*z+1, -SEP*y-1] })
                        else:
                            gltf['nodes'].append({
                                "name":f"Turn-X&-Y({x},{y},{z})",
                                "mesh": 8,
                                "rotation": [0.5, -0.5, -0.5, 0.5],
                                "translation": [SEP*x, SEP*z+1, -SEP*y-1] })
                    
                    if exists['-y'] and exists['+z']:
                        if ColorY[x][y-1][z]:
                            gltf['nodes'].append({
                                "name":f"Turn-Y&+Z({x},{y},{z})",
                                "mesh": 7,
                                "rotation": [1, 0, 0, 0],
                                "translation": [SEP*x, SEP*z, -SEP*y] })
                        else:
                            gltf['nodes'].append({
                                "name":f"Turn-Y&+Z({x},{y},{z})",
                                "mesh": 8,
                                "rotation": [1, 0, 0, 0],
                                "translation": [SEP*x, SEP*z, -SEP*y] })
                            
                    if exists['+y'] and exists['-z']:
                        if ColorY[x][y][z]:
                            gltf['nodes'].append({
                                "name":f"Turn+Y&-Z({x},{y},{z})",
                                "mesh": 7,
                                "translation": [SEP*x, SEP*z+1, -SEP*y-1] })
                        else:
                            gltf['nodes'].append({
                                "name":f"Turn+Y&-Z({x},{y},{z})",
                                "mesh": 8,
                                "translation": [SEP*x, SEP*z+1, -SEP*y-1] })
                            
                    if exists['+y'] and exists['+z']:
                        if ColorY[x][y][z]:
                            gltf['nodes'].append({
                                "name":f"Turn+Y&+Z({x},{y},{z})",
                                "mesh": 7,
                                "rotation": [0, 1, 0, 0],
                                "translation": [SEP*x+1, SEP*z, -SEP*y-1] })
                        else:
                            gltf['nodes'].append({
                                "name":f"Turn+Y&+Z({x},{y},{z})",
                                "mesh": 8,
                                "rotation": [0, 1, 0, 0],
                                "translation": [SEP*x+1, SEP*z, -SEP*y-1] })
                    
                    if exists['-y'] and exists['-z']:
                        if ColorY[x][y-1][z]:
                            gltf['nodes'].append({
                                "name":f"Turn-Y&-Z({x},{y},{z})",
                                "mesh": 7,
                                "rotation": [SQ2, 0, 0, SQ2],
                                "translation": [SEP*x, SEP*z, -SEP*y-1] })
                        else:
                            gltf['nodes'].append({
                                "name":f"Turn-Y&-Z({x},{y},{z})",
                                "mesh": 8,
                                "rotation": [SQ2, 0, 0, SQ2],
                                "translation": [SEP*x, SEP*z, -SEP*y-1] })
                     
                    if exists['+x'] and exists['+z']:
                        if ColorX[x][y][z]:
                            gltf['nodes'].append({
                                "name":f"Turn+X&+Z({x},{y},{z})",
                                "mesh": 8,
                                "rotation": [0, SQ2, 0, SQ2],
                                "translation": [SEP*x+1, SEP*z, -SEP*y] })
                        else:
                            gltf['nodes'].append({
                                "name":f"Turn+X&+Z({x},{y},{z})",
                                "mesh": 7,
                                "rotation": [0, SQ2, 0, SQ2],
                                "translation": [SEP*x+1, SEP*z, -SEP*y] })
                            
                    if exists['-x'] and exists['+z']:
                        if ColorX[x-1][y][z]:
                            gltf['nodes'].append({
                                "name":f"Turn-X&+Z({x},{y},{z})",
                                "mesh": 8,
                                "rotation": [0, -SQ2, 0, SQ2],
                                "translation": [SEP*x, SEP*z, -SEP*y-1] })
                        else:
                            gltf['nodes'].append({
                                "name":f"Turn-X&+Z({x},{y},{z})",
                                "mesh": 7,
                                "rotation": [0, -SQ2, 0, SQ2],
                                "translation": [SEP*x, SEP*z, -SEP*y-1] })
                            
                    if exists['+x'] and exists['-z']:
                        if ColorX[x][y][z]:
                            gltf['nodes'].append({
                                "name":f"Turn+X&-Z({x},{y},{z})",
                                "mesh": 8,
                                "rotation": [0.5, 0.5, -0.5, 0.5],
                                "translation": [SEP*x, SEP*z, -SEP*y] })
                        else:
                            gltf['nodes'].append({
                                "name":f"Turn+X&-Z({x},{y},{z})",
                                "mesh": 7,
                                "rotation": [0.5, 0.5, -0.5, 0.5],
                                "translation": [SEP*x, SEP*z, -SEP*y] })
                    
                    if exists['-x'] and exists['-z']:
                        if ColorX[x-1][y][z]:
                            gltf['nodes'].append({
                                "name":f"Turn-X&-Z({x},{y},{z})",
                                "mesh": 8,
                                "rotation": [0.5, -0.5, 0.5, 0.5],
                                "translation": [SEP*x+1, SEP*z, -SEP*y-1] })
                        else:
                            gltf['nodes'].append({
                                "name":f"Turn-X&-Z({x},{y},{z})",
                                "mesh": 7,
                                "rotation": [0.5, -0.5, 0.5, 0.5],
                                "translation": [SEP*x+1, SEP*z, -SEP*y-1] })

            elif degree == 3:
                if exists['-y'] and exists['+y'] and exists['+z']:
                    if ColorY[x][y][z]:
                        gltf['nodes'].append({
                            "name":f"JuncY&+Z({x},{y},{z})",
                            "mesh": 9,
                            "translation": [SEP*x, SEP*z, -SEP*y] })
                    else:
                        gltf['nodes'].append({
                            "name":f"JuncY&+Z({x},{y},{z})",
                            "mesh": 10,
                            "translation": [SEP*x, SEP*z, -SEP*y] })
                
                if exists['-y'] and exists['+y'] and exists['-z']:
                    if ColorY[x][y][z]:
                        gltf['nodes'].append({
                            "name":f"JuncY&-Z({x},{y},{z})",
                            "mesh": 9,
                            "rotation": [1, 0, 0, 0],
                            "translation": [SEP*x, SEP*z+1, -SEP*y-1] })
                    else:
                        gltf['nodes'].append({
                            "name":f"JuncY&+Z({x},{y},{z})",
                            "mesh": 10,
                            "translation": [SEP*x, SEP*z+1, -SEP*y-1] })
                        
                if exists['-z'] and exists['+z'] and exists['-y']:
                    if ColorY[x][y-1][z]:
                        gltf['nodes'].append({
                            "name":f"JuncZ&-Y({x},{y},{z})",
                            "mesh": 9,
                            "rotation": [SQ2, 0, 0, SQ2],
                            "translation": [SEP*x, SEP*z, -SEP*y-1] })
                    else:
                        gltf['nodes'].append({
                            "name":f"JuncZ&-Y({x},{y},{z})",
                            "mesh": 10,
                            "rotation": [SQ2, 0, 0, SQ2],
                            "translation": [SEP*x, SEP*z, -SEP*y-1] })
                        
                if exists['-z'] and exists['+z'] and exists['+y']:
                    if ColorY[x][y][z]:
                        gltf['nodes'].append({
                            "name":f"JuncZ&+Y({x},{y},{z})",
                            "mesh": 9,
                            "rotation": [-SQ2, 0, 0, SQ2],
                            "translation": [SEP*x, SEP*z+1, -SEP*y] })
                    else:
                        gltf['nodes'].append({
                            "name":f"JuncZ&+Y({x},{y},{z})",
                            "mesh": 10,
                            "rotation": [-SQ2, 0, 0, SQ2],
                            "translation": [SEP*x, SEP*z+1, -SEP*y] })

                if exists['-y'] and exists['+y'] and exists['-x']:
                    if ColorX[x-1][y][z]:
                        gltf['nodes'].append({
                            "name":f"JuncY&-X({x},{y},{z})",
                            "mesh": 9,
                            "rotation": [0, 0, SQ2, SQ2],
                            "translation": [SEP*x+1, SEP*z, -SEP*y] })
                    else:
                        gltf['nodes'].append({
                            "name":f"JuncY&-X({x},{y},{z})",
                            "mesh": 10,
                            "rotation": [0, 0, SQ2, SQ2],
                            "translation": [SEP*x+1, SEP*z, -SEP*y] })
                
                if exists['-y'] and exists['+y'] and exists['+x']:
                    if ColorX[x][y][z]:
                        gltf['nodes'].append({
                            "name":f"JuncY&+X({x},{y},{z})",
                            "mesh": 9,
                            "rotation": [0, 0, -SQ2, SQ2],
                            "translation": [SEP*x, SEP*z+1, -SEP*y] })
                    else:
                        gltf['nodes'].append({
                            "name":f"JuncY&+X({x},{y},{z})",
                            "mesh": 10,
                            "rotation": [0, 0, -SQ2, SQ2],
                            "translation": [SEP*x, SEP*z+1, -SEP*y] })

                if exists['-x'] and exists['+x'] and exists['-y']:
                    if ColorX[x][y][z]:
                        gltf['nodes'].append({
                            "name":f"JuncX&-Y({x},{y},{z})",
                            "mesh": 9,
                            "rotation": [0.5, 0.5, 0.5, 0.5],
                            "translation": [SEP*x+1, SEP*z, -SEP*y-1] })
                    else:
                        gltf['nodes'].append({
                            "name":f"JuncX&-Y({x},{y},{z})",
                            "mesh": 10,
                            "rotation": [0.5, 0.5, 0.5, 0.5],
                            "translation": [SEP*x+1, SEP*z, -SEP*y-1] })
                        
                if exists['-x'] and exists['+x'] and exists['+y']:
                    if ColorX[x][y][z]:
                        gltf['nodes'].append({
                            "name":f"JuncX&+Y({x},{y},{z})",
                            "mesh": 9,
                            "rotation": [-0.5, 0.5, -0.5, 0.5],
                            "translation": [SEP*x+1, SEP*z+1, -SEP*y] })
                    else:
                        gltf['nodes'].append({
                            "name":f"JuncX&+Y({x},{y},{z})",
                            "mesh": 10,
                            "rotation": [-0.5, 0.5, -0.5, 0.5],
                            "translation": [SEP*x+1, SEP*z+1, -SEP*y] })
                        
                if exists['-x'] and exists['+x'] and exists['+z']:
                    if ColorX[x][y][z]:
                        gltf['nodes'].append({
                            "name":f"JuncX&+Z({x},{y},{z})",
                            "mesh": 10,
                            "rotation": [0, SQ2, 0, SQ2],
                            "translation": [SEP*x+1, SEP*z, -SEP*y] })
                    else:
                        gltf['nodes'].append({
                            "name":f"JuncX&+Z({x},{y},{z})",
                            "mesh": 9,
                            "rotation": [0, SQ2, 0, SQ2],
                            "translation": [SEP*x+1, SEP*z, -SEP*y] })
                        
                if exists['-x'] and exists['+x'] and exists['-z']:
                    if ColorX[x][y][z]:
                        gltf['nodes'].append({
                            "name":f"JuncX&-Z({x},{y},{z})",
                            "mesh": 10,
                            "rotation": [SQ2, 0, SQ2, 0],
                            "translation": [SEP*x+1, SEP*z+1, -SEP*y] })
                    else:
                        gltf['nodes'].append({
                            "name":f"JuncX&-Z({x},{y},{z})",
                            "mesh": 9,
                            "rotation": [SQ2, 0, SQ2, 0],
                            "translation": [SEP*x+1, SEP*z+1, -SEP*y] })
                        
                if exists['-z'] and exists['+z'] and exists['+x']:
                    if ColorX[x][y][z]:
                        gltf['nodes'].append({
                            "name":f"JuncZ&+X({x},{y},{z})",
                            "mesh": 10,
                            "rotation": [0.5, 0.5, -0.5, 0.5],
                            "translation": [SEP*x, SEP*z, -SEP*y] })
                    else:
                        gltf['nodes'].append({
                            "name":f"JuncZ&+X({x},{y},{z})",
                            "mesh": 9,
                            "rotation": [0.5, 0.5, -0.5, 0.5],
                            "translation": [SEP*x, SEP*z, -SEP*y] })
                        
                if exists['-z'] and exists['+z'] and exists['-x']:
                    if ColorX[x-1][y][z]:
                        gltf['nodes'].append({
                            "name":f"JuncZ&-X({x},{y},{z})",
                            "mesh": 10,
                            "rotation": [0.5, -0.5, 0.5, 0.5],
                            "translation": [SEP*x+1, SEP*z, -SEP*y-1] })
                    else:
                        gltf['nodes'].append({
                            "name":f"JuncZ&-X({x},{y},{z})",
                            "mesh": 9,
                            "rotation": [0.5, -0.5, 0.5, 0.5],
                            "translation": [SEP*x+1, SEP*z, -SEP*y-1] })
                
            elif degree == 4:
                if exists['-z'] and exists['+z'] and exists['-y'] and exists['+y']:
                    if ColorY[x][y][z]:
                        gltf['nodes'].append({
                            "name":f"CrossYZ({x},{y},{z})",
                            "mesh": 11,
                            "translation": [SEP*x, SEP*z, -SEP*y] })
                    else:
                        gltf['nodes'].append({
                            "name":f"CrossYZ({x},{y},{z})",
                            "mesh": 12,
                            "translation": [SEP*x, SEP*z, -SEP*y] })
                        
                if exists['-x'] and exists['+x'] and exists['-y'] and exists['+y']:
                    if ColorX[x][y][z]:
                        gltf['nodes'].append({
                            "name":f"CrossXY({x},{y},{z})",
                            "mesh": 11,
                            "translation": [SEP*x+1, SEP*z, -SEP*y] })
                    else:
                        gltf['nodes'].append({
                            "name":f"CrossXY({x},{y},{z})",
                            "mesh": 12,
                            "translation": [SEP*x+1, SEP*z, -SEP*y] })
                        
                if exists['-x'] and exists['+x'] and exists['-z'] and exists['+z']:
                    if ColorX[x][y][z]:
                        gltf['nodes'].append({
                            "name":f"CrossXZ({x},{y},{z})",
                            "mesh": 12,
                            "translation": [SEP*x+1, SEP*z, -SEP*y] })
                    else:
                        gltf['nodes'].append({
                            "name":f"CrossXZ({x},{y},{z})",
                            "mesh": 11,
                            "translation": [SEP*x+1, SEP*z, -SEP*y] })

            elif degree >= 5:
                raise ValueError('Have 3D corner!')
            
gltf['nodes'][0]['children'] = list(range(1, len(gltf['nodes'])))
with open(OLSCO_FILE_NAME.replace('.json', '.gltf'), 'w') as f:
    json.dump(gltf, f)