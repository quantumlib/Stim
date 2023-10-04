import codecs
import struct
import json

SQ2 = 0.70710678118

def float_to_little_endian_hex(f):
    # Pack the float into a binary string using the little-endian format
    binary_data = struct.pack('<f', f)
    
    # Convert the binary string to a hexadecimal string
    hex_string = binary_data.hex()
    
    return hex_string

def binary_gen(tubelen:float=2.0):
	# little endian
	floats = {'0':'00000000', '1':'0000803F', '-1':'000080BF'}
	floats['tube'] = float_to_little_endian_hex(tubelen)
	ints = ['0000', '0100', '0200', '0300', '0400', '0500', '0600', '0700']

	full_bin = ''

	# convention: (X, Z, -Y)
	positions = ''
	# position of points for the cube
	positions += floats['0'] + floats['0'] + floats['0']
	positions += floats['1'] + floats['0'] + floats['0']
	positions += floats['0'] + floats['0'] + floats['-1']
	positions += floats['1'] + floats['0'] + floats['-1']

	# position fo points for the tube
	positions += floats['0'] + floats['0'] + floats['0']
	positions += floats['tube'] + floats['0'] + floats['0']
	positions += floats['0'] + floats['0'] + floats['-1']
	positions += floats['tube'] + floats['0'] + floats['-1']

	full_bin += positions


	normals = ''
	# normals -Z
	for _ in range(4):
		normals += floats['0'] + floats['-1'] + floats['0']
	
	full_bin += normals

	# vertices for -Z
	vertices = ints[1] + ints[0] + ints[3] + ints[3] + ints[0] + ints[2]
	full_bin += vertices

	# for the Hadamard edges
	full_bin += floats['0'] + floats['0'] + floats['0']
	full_bin += floats['tube'] + floats['0'] + floats['0']
	full_bin += floats['0'] + floats['0'] + floats['-1']
	full_bin += floats['tube'] + floats['0'] + floats['-1']
	full_bin += float_to_little_endian_hex(tubelen*5/8) + floats['0'] + floats['0']
	full_bin += float_to_little_endian_hex(tubelen*3/8) + floats['0'] + float_to_little_endian_hex(-0.5)
	full_bin += float_to_little_endian_hex(tubelen*5/8) + floats['0'] + floats['-1']

	for _ in range(7):
		full_bin += floats['0'] + floats['-1'] + floats['0']

	full_bin += ints[0] + ints[2] + ints[5]
	full_bin += ints[4] + ints[0] + ints[5]
	full_bin += ints[5] + ints[2] + ints[6]
	full_bin += ints[4] + ints[5] + ints[1]
	full_bin += ints[1] + ints[5] + ints[3]
	full_bin += ints[3] + ints[5] + ints[6]

	full_bin_b64 = codecs.encode(codecs.decode(full_bin, 'hex'), 'base64').decode()
	full_bin_b64 = str(full_bin_b64).replace('\n', '')
	return [{"byteLength":360,
			 "uri":"data:application/octet-stream;base64,"+full_bin_b64}]

def tube_gen(SEP, loc, dir, color=None):
	if dir == 'I':
		rectangles = [
			{
				"name":f"edgeI{loc}:-K",
				"translation": [1+SEP*loc[0], SEP*loc[2], -SEP*loc[1]] 
			},
			{
				"name":f"edgeI{loc}:+K",
				"translation": [1+SEP*loc[0], 1+SEP*loc[2], -SEP*loc[1]] 
			},
			{
				"name":f"edgeI{loc}:-J",
				"translation": [1+SEP*loc[0], SEP*loc[2], -SEP*loc[1]],
				"rotation": [SQ2, 0, 0, SQ2]
			},
			{
				"name":f"edgeI{loc}:+J",
				"translation": [1+SEP*loc[0], SEP*loc[2], -1-SEP*loc[1]],
				"rotation": [SQ2, 0, 0, SQ2]
			}
		]
		if color == 0:
			rectangles[0]["mesh"] = 5
			rectangles[1]["mesh"] = 5
			rectangles[2]["mesh"] = 4
			rectangles[3]["mesh"] = 4
		elif color == 1:
			rectangles[0]["mesh"] = 4
			rectangles[1]["mesh"] = 4
			rectangles[2]["mesh"] = 5
			rectangles[3]["mesh"] = 5
		else:
			raise ValueError(f'No such color: {color}')
	elif dir == 'J':
		rectangles = [
			{
				"name":f"edgeJ{loc}:-K",
				"rotation": [0, SQ2, 0, SQ2],
				"translation": [1+SEP*loc[0], SEP*loc[2], -1-SEP*loc[1]] 
			},
			{
				"name":f"edgeJ{loc}:+K",
				"rotation": [0, SQ2, 0, SQ2],
				"translation": [1+SEP*loc[0], 1+SEP*loc[2], -1-SEP*loc[1]] 
			},
			{
				"name":f"edgeJ{loc}:-I",
				"rotation": [0.5, 0.5, -0.5, 0.5],
				"translation": [SEP*loc[0], SEP*loc[2], -1-SEP*loc[1]]
			},
			{
				"name":f"edgeJ{loc}:+I",
				"rotation": [0.5, 0.5, -0.5, 0.5],
				"translation": [1+SEP*loc[0], SEP*loc[2], -1-SEP*loc[1]]
			}
		]
		if color == 0:
			rectangles[0]["mesh"] = 4
			rectangles[1]["mesh"] = 4
			rectangles[2]["mesh"] = 5
			rectangles[3]["mesh"] = 5
		elif color == 1:
			rectangles[0]["mesh"] = 5
			rectangles[1]["mesh"] = 5
			rectangles[2]["mesh"] = 4
			rectangles[3]["mesh"] = 4
		else:
			raise ValueError(f'No such color: {color}')
	elif dir == 'K':
		rectangles = [
			{
				"name":f"edgeK{loc}:-J",
				"rotation": [0.5, 0.5, 0.5, 0.5],
				"translation": [1+SEP*loc[0], 1+SEP*loc[2], -SEP*loc[1]] 
			},
			{
				"name":f"edgeJ{loc}:+J",
				"rotation": [0.5, 0.5, 0.5, 0.5],
				"translation": [1+SEP*loc[0], 1+SEP*loc[2], -1-SEP*loc[1]] 
			},
			{
				"name":f"edgeJ{loc}:-I",
				"rotation": [0, 0, SQ2, SQ2],
				"translation": [SEP*loc[0], 1+SEP*loc[2], -SEP*loc[1]]
			},
			{
				"name":f"edgeJ{loc}:+I",
				"rotation": [0, 0, SQ2, SQ2],
				"translation": [1+SEP*loc[0], 1+SEP*loc[2], -SEP*loc[1]]
			}
		]
		rectangles[0]["mesh"] = 6
		rectangles[1]["mesh"] = 6
		rectangles[2]["mesh"] = 6
		rectangles[3]["mesh"] = 6
	else:
		raise ValueError(f'No such direction of edge: {dir}')
	
	return rectangles
	
def cube_gen(SEP, edges):
	return

def gltf_gen(lasir_file:str,
	     	 gltf_file:str,
	     	 tubelen:float=2.0,
			 base_gltf_file='./base.gltf'):
	with open(lasir_file, 'r') as f:
		lasir = json.load(f)
	with open(base_gltf_file, 'r') as f:
		gltf = json.load(f)
	gltf["buffers"] = binary_gen(tubelen)
	

	gltf['nodes'] = [{"name":"smaple"},]
	i_bound = lasir['i_bound']
	j_bound = lasir['j_bound']
	k_bound = lasir['k_bound']
	NodeY = lasir['NodeY']
	ExistI = lasir['ExistI']
	ColorI = lasir['ColorI']
	ExistJ = lasir['ExistJ']
	ColorJ = lasir['ColorJ']
	ExistK = lasir['ExistK']
	for i in range(i_bound):
		for j in range(j_bound):
			for k in range(k_bound):
				if ExistI[i][j][k]:
					gltf['nodes'] += tube_gen(tubelen+1.0, (i,j,k), 'I', ColorI[i][j][k])
				if ExistJ[i][j][k]:
					gltf['nodes'] += tube_gen(tubelen+1.0, (i,j,k), 'J', ColorJ[i][j][k])
				if ExistK[i][j][k]:
					gltf['nodes'] += tube_gen(tubelen+1.0, (i,j,k), 'K')


	gltf['nodes'][0]['children'] = list(range(1, len(gltf['nodes'])))
	with open(gltf_file, 'w') as f:
		json.dump(gltf, f)