import json

def olsco_verify(olsco):
	pass
	
def lasir_gen(olsco_file:str, output_file:str):
    with open(olsco_file, 'r') as f:
        olsco = json.load(f)
    olsco_verify(olsco)
    lasir = olsco
    with open(output_file, 'w') as f:
        json.dump(lasir, f)