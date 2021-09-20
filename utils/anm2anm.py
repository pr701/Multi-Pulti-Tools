#!/usr/bin/env python
#
# MediaHouse animation v1 to v2 converter
#
# Authors:	painter
# Version:	1.0.0.1
# License:	GPL v3
#
# History:
# 1.0		2020-08-04 First version.

import os
import argparse

from os import listdir
from os.path import isfile, isdir, join
from struct import *
from collections import namedtuple

anm_header_t = '<2s3H'
sequince_v1_t = '<21s4H3L'
sequince_v2_t = '<41s4H3L'

anm_header = namedtuple('anm_header', 'magic width hight sequinceCount')
anm_seq = namedtuple('anm_seq', 'name frameCount frameSpeed reserved1 reserved2 frameOffset dataOffset chunkSize')


def convertAnmToAnm2(inFile, outFile):
	with open(inFile, 'rb') as f:
		try:
			header = anm_header._make(unpack(anm_header_t, f.read(calcsize(anm_header_t))))

			if header.magic != b'AN':
				raise argparse.ArgumentTypeError('Invalid file format')

			headerSize = 0x421
			f.seek(0)
			hcolors = f.read(headerSize)

			# rebuild table v1 to v2
			offset = 20 * header.sequinceCount
			table = b''
			for i in range(0, header.sequinceCount):
				f.seek(headerSize + i * calcsize(sequince_v1_t))
				seq = anm_seq._make(unpack(sequince_v1_t, f.read(calcsize(sequince_v1_t))))
				table += pack(sequince_v2_t, seq.name, seq.frameCount, seq.frameSpeed,
					seq.reserved1, seq.reserved2, seq.frameOffset + offset, seq.dataOffset + offset, seq.chunkSize)

			f.seek(headerSize + header.sequinceCount * calcsize(sequince_v1_t))
			frames = f.read()

			if not len(table):
				raise argparse.ArgumentTypeError('Invalid table')

			# rebuild
			with open(outFile, 'wb') as o:
				try:
					o.write(hcolors + table + frames)
					print('%s : ok' % inFile)
				except Exception:
					raise
				finally:
					o.close()

		except Exception as e:
			raise Exception(e)
		finally:
			f.close()

# main


parser = argparse.ArgumentParser(prog='libex', description='MediaHouse libs resource unpacker/packer')

parser.add_argument('-i', '--input', action='store', type=str, required=True,
					help='source directory or animation file')
parser.add_argument('-o', '--output', action='store', type=str, required=True,
					help='output directory or animation file')

args = parser.parse_args()

try:
	if isdir(args.input):
		if not isdir(args.output):
			os.makedirs(args.output)

		files = [f for f in listdir(args.input) if isfile(join(args.input, f))]
		for file in files:
			try:
				convertAnmToAnm2(join(args.input, file), join(args.output, file))
			except Exception as e:
				print('%s : skipped, %s' % (file, e))
	elif isfile(args.input):
		try:
			convertAnmToAnm2(args.input, args.output)
		except Exception as e:
			print('%s : skipped, %s' % (args.input, e))
			exit(1)
	else:
		raise argparse.ArgumentTypeError('the input does not exist')
except Exception as e:
	print('error: %s' % e)
else:
	print('done')
