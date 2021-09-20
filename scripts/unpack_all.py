# Unpack all disc data to fodler

import sys
import os
import errno


def createDir(dir):
	try:
		os.makedirs(dir)
	except OSError as e:
		if e.errno != errno.EEXIST:
			raise


if len(sys.argv) != 3:
	print('Required set IN and OUT directory')
	exit(2)

# common paths
libdir = os.path.join(sys.argv[1], 'BMP')
outdir = sys.argv[2]

if not os.path.isdir(libdir):
	print('Invalid IN directory')
	exit(2)

createDir(outdir)
if not os.path.isdir(outdir):
	print('Invalid OUT directory')
	exit(2)

print('Extracting...')

lib_list = {'ANM', 'ANM_s', 'FON', 'FON_S',
	'STAT', 'STAT_S', 'WAV_EF', 'WAV_MUS'} # 'WAV_NRT', 'WAV_TIR' is fake archives, 'WAV_LG' is dublicates

for name in lib_list:
	file = os.path.join(libdir, name + '.lib')
	out = os.path.join(outdir, name)
	createDir(out)
	if os.path.isfile(file) and os.path.isdir(out):
		os.system('libex.py unpack -i "%s" -o "%s" > log.txt' % (file, out))

print('Done!')
