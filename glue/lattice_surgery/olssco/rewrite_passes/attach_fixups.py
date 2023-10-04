# assuming all T injections requiring fixup are on the top floor. The output is also on the top floor


def attach_fixups(lasir):
  n_s = len(lasir['CorrIJ'])
  n_i = lasir['n_i']
  n_j = lasir['n_j']
  n_k = lasir['n_k']

  fixup_locs = []
  for p in lasir['optional']['top_fixups']:
    fixup_locs.append((lasir['ports'][p]['i'], lasir['ports'][p]['j']))

  for i in range(n_i):
    for j in range(n_j):
      if (i, j) in fixup_locs:
        lasir['NodeY'][i][j].append(1)
        lasir['ExistK'][i][j][n_k - 1] = 1
      else:
        lasir['NodeY'][i][j].append(0)
        lasir['ExistK'][i][j][n_k - 1] = 0
      lasir['NodeY'][i][j].append(0)
      for arr in ['ExistI', 'ExistJ', 'ExistK']:
        lasir[arr][i][j].append(0)
        lasir[arr][i][j].append(0)
      for arr in ['ColorI', 'ColorJ', 'ColorKM', 'ColorKP']:
        lasir[arr][i][j].append(-1)
        lasir[arr][i][j].append(-1)
      for s in range(n_s):
        for arr in ['CorrIJ', 'CorrIK', 'CorrJK', 'CorrJI', 'CorrKI', 'CorrKJ']:
          lasir[arr][s][i][j].append(0)
          lasir[arr][s][i][j].append(0)

  for port in lasir['ports']:
    if 'f' in port and port['f'] == 'output':
      ii, jj = port['i'], port['j']
      lasir['ExistK'][ii][jj][n_k - 1] = 1
      lasir['ExistK'][ii][jj][n_k] = 1
      lasir['ExistK'][ii][jj][n_k + 1] = 1
      lasir['ColorKM'][ii][jj][n_k] = lasir['ColorKP'][ii][jj][n_k - 1]
      lasir['ColorKM'][ii][jj][n_k + 1] = lasir['ColorKM'][ii][jj][n_k]
      lasir['ColorKP'][ii][jj][n_k] = lasir['ColorKM'][ii][jj][n_k]
      lasir['ColorKP'][ii][jj][n_k + 1] = lasir['ColorKP'][ii][jj][n_k]
      for s in range(n_s):
        lasir['CorrKI'][s][ii][jj][n_k] = lasir['CorrKI'][s][ii][jj][n_k - 1]
        lasir['CorrKI'][s][ii][jj][n_k + 1] = lasir['CorrKI'][s][ii][jj][n_k]
        lasir['CorrKJ'][s][ii][jj][n_k] = lasir['CorrKJ'][s][ii][jj][n_k - 1]
        lasir['CorrKJ'][s][ii][jj][n_k + 1] = lasir['CorrKJ'][s][ii][jj][n_k]
      port['k'] += 2
      for c in lasir['port_cubes']:
        if c[0] == port['i'] and c[1] == port['j']:
          c[2] = port['k'] + 1

  t_injections = []
  for port in lasir['ports']:
    if port['f'] == 'T':
      if port['e'] == '+':
        t_injections.append([port['i'], port['j'], port['k'] + 1])
      else:
        t_injections.append([port['i'], port['j'], port['k']])
  lasir['optional']['t_injections'] = t_injections

  lasir['n_k'] += 2
  return lasir
