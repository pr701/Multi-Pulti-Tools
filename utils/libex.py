#!/usr/bin/env python
#
# MediaHouse libs resource unpacker/packer
#
# Authors:	painter
# Version:	1.0.0.5
# License:	GPL v3
#
# History:
# 1.0.5		2020-07-30 Added RAW mode.
# 1.0.4		2020-07-29 Implimented both version support.
# 1.0.3		2020-07-29 Implimented WAV converter.
# 1.0.2		2020-07-29 Implimented BMP converter.
# 1.0.1		2020-07-28 Implimented packer.
# 1.0		2020-07-27 First version.

import os
import argparse
import re

from os import listdir
from os.path import isfile, isdir, join
from struct import *
from collections import namedtuple

# common defines
lib_header_t = '<L2hL'
lib_table_t = '<31s2L' # v2, sample

bmp_header_t = '<2sL2HL'
bmp_info_header_t = '<3L2H6L'
wav_header_t = '4sL4s4sL2H2L2H4sL'
wav_custom_header_t = '<2H2L2H2s'

lib_header = namedtuple('lib_header', 'magic itemCount version tableOffset')
lib_table = namedtuple('lib_table', 'filename dataOffset dataSize')
bmp_header = namedtuple('bmp_header', 'type size reserved1 reserved2 offbits')
bmp_info_header = namedtuple('bmp_info_header', 'size width height planes \
	bitCount compression sizeImage xPelsPerMeter yPelsPerMeter clrUsed clrImportant')
wav_header = namedtuple('wav_custom_header', 'groupId size riffType chunkId chunkSize \
	formatTag channels sampleRate bitRate blockAlign bitPerSample dataId dataSize')
wav_custom_header = namedtuple('wav_custom_header', 'formatTag channels sampleRate \
	bitRate blockAlign bitPerSample dataId')

magic_variant = {'bmp_v1': 0xC90064,
				'bmp_v2': 0xCA0065,
				'anm_v1': 0xCA0066,
				'anm_v2': 0xCB0066,
				'anm_v3': 0xC90066,
				'wav_v1': 0xCC0067}

# utils
digits = re.compile(r'(\d+)')


def tokenize(filename):
	return tuple(int(token) if match else token
				for token, match in
				((fragment, digits.search(fragment))
				for fragment in digits.split(filename)))


def magic_to_str(number):
	for i, v in enumerate(magic_variant):
		if number == magic_variant[v]:
			return v
	return 'unk'


def str_to_magic(str):
	for i, v in enumerate(magic_variant):
		if str.lower() == v:
			return magic_variant[v]
	return 0


def is_magic(number, type_str):
	for i, v in enumerate(magic_variant):
		if number == magic_variant[v] and v[:3] == type_str:
			return True
	return False


def get_file_size(file):
	file.seek(0, 2)
	size = file.tell()
	return size


def addExtension(filename, ext):
	ext = '.' + ext
	if not filename.endswith(ext.lower()) and not filename.endswith(ext.upper()):
		return filename + ext.lower()
	else:
		return filename


def removeExtension(name):
	if len(name) > 5 and name[:-3].endswith('.'):
		return name[:-4]
	return name


def fromNullTerminated(name):
	for i in range(len(name)):
		if name[i] == 0:
			return name[:i]
	return name


def fixName(name):
	for c in '<>:"/\\|?*\x0C':
		name = name.replace(c, '')
	return name

# format converters


def restore_bmp_header(raw):
	size = calcsize(bmp_header_t) + len(raw)
	info = bmp_info_header._make(unpack(bmp_info_header_t, raw[:calcsize(bmp_info_header_t)]))
	offs = calcsize(bmp_header_t) + calcsize(bmp_info_header_t)

	if info.bitCount != 24 and info.bitCount != 32:
		if info.clrUsed > 0:
			offs += 4 * info.clrUsed
		else:
			offs += 4 * (1 << info.bitCount)

	return pack(bmp_header_t, *bmp_header(b'BM', size, 0, 0, offs)) + raw


def remove_bmp_header(raw):
	return raw[calcsize(bmp_header_t):]


def restore_wav_header(raw):
	# parse internal
	header = wav_custom_header._make(unpack(wav_custom_header_t, raw[:calcsize(wav_custom_header_t)]))
	raw = raw[calcsize(wav_custom_header_t):]

	# rebuild header
	return pack(wav_header_t, b'RIFF', len(raw) + 24, b'WAVE',
		b'fmt\x20', 0x10, header.formatTag, header.channels, header.sampleRate, header.bitRate,
		header.blockAlign, header.bitPerSample, b'data', len(raw)) + raw


def remove_wav_header(raw):
	header = wav_header._make(unpack(wav_header_t, raw[:calcsize(wav_header_t)]))
	raw = raw[calcsize(wav_header_t):]

	# rebuild to internal
	return pack(wav_custom_header_t, header.formatTag, header.channels,
		header.sampleRate, header.bitRate, header.blockAlign, header.bitPerSample, b'da') + raw

# impl


def unpack_lib(libfile, outdir, raw):
	if not isdir(outdir):
		os.makedirs(outdir)

	with open(libfile, 'rb') as f:
		try:
			header = lib_header._make(unpack(lib_header_t, f.read(calcsize(lib_header_t))))

			if magic_to_str(header.magic) == 'unk':
				raise argparse.ArgumentTypeError('unsupported library type')

			# print header info
			print('Unpack lib: %s' % os.path.basename(libfile))
			print('Type: %s' % magic_to_str(header.magic))
			print('Version: %i' % header.version)
			print('Files count: %i' % header.itemCount)
			print('Raw mode:', raw)

			# index table
			filesize = get_file_size(f)
			if (header.tableOffset >= filesize):
				raise argparse.ArgumentTypeError('Invalid index table offset')

			# update struct type
			if (header.version == 0):
				filename_maxlen = 21
			else:
				filename_maxlen = 31

			# check entry
			f.seek(header.tableOffset + filename_maxlen)
			if unpack('<L', f.read(4))[0] != 0xC:
				raise argparse.ArgumentTypeError('Invalid index table entry')

			print('Extracting...')

			# read table
			lib_table_t = '<%ss2L' % filename_maxlen
			for i in range(0, header.itemCount):
				f.seek(header.tableOffset + i * calcsize(lib_table_t))
				table = lib_table._make(unpack(lib_table_t, f.read(calcsize(lib_table_t))))
				file = fixName(fromNullTerminated(table.filename).decode('cp1251'))

				# add extension?
				if not raw:
					ext = magic_to_str(header.magic)[:3]
					file = addExtension(file, ext)

				# extract
				outpath = os.path.join(outdir, file)
				with open(outpath, 'wb') as o:
					try:
						# get data
						f.seek(table.dataOffset)
						data = f.read(table.dataSize)

						# save as is
						if raw:
							o.write(data)
						# convert to normal bmp
						elif is_magic(header.magic, 'bmp'):
							o.write(restore_bmp_header(data))
						# convert to normal wav
						elif is_magic(header.magic, 'wav'):
							o.write(restore_wav_header(data))
						# animations as is
						else:
							o.write(data)

						print('%s : ok' % file)
					except Exception as e:
						print('%s : skipped, %s' % (file, e))
					finally:
						o.close()
			print('Done')

		except Exception as e:
			parser.error(e)
		finally:
			f.close()


def pack_lib(type, version, indir, libfile, raw=False, srtict=True):

	if not isdir(indir):
		raise argparse.ArgumentTypeError('the source directory does not exist')

	libfile = addExtension(libfile, 'lib')
	magic = str_to_magic(type)

	fileCount = 0
	files = [f for f in listdir(indir) if isfile(join(indir, f))]
	files.sort(key=tokenize)

	table = b''
	data = b''
	offset = calcsize(lib_header_t)

	# update table structure
	if (version == 0):
		filename_maxlen = 21
	else:
		filename_maxlen = 31
	lib_table_t = '<%ss2L' % filename_maxlen

	print('Processing...')
	for file in files:
		# prepare filename to index table
		if raw:
			nameInTable = file
		elif is_magic(magic, 'bmp'):
			nameInTable = addExtension(file, 'bmp')
		else:
			nameInTable = removeExtension(file)

		if len(nameInTable) > filename_maxlen:
			if strict:
				nameInTable = nameInTable[:filename_maxlen - 1]
			else:
				print('%s : skipped, file name too long' % file)
				continue

		payload = b''
		with open(join(indir, file), 'rb') as f:
			try:
				# validate bmp
				if not raw and is_magic(magic, 'bmp'):
					header = bmp_header._make(unpack(bmp_header_t, f.read(calcsize(bmp_header_t))))
					if header.type != b'BM':
						raise Exception('invalid file type (BMP)')
					info_header = bmp_info_header._make(unpack(bmp_info_header_t, f.read(calcsize(bmp_info_header_t))))
					if info_header.compression > 0:
						raise Exception('compression is unsupported')
					f.seek(0)
					payload = remove_bmp_header(f.read())
				# validate wav
				elif not raw and is_magic(magic, 'wav'):
					header = wav_header._make(unpack(wav_header_t, f.read(calcsize(wav_header_t))))
					if header.groupId != b'RIFF' or header.riffType != b'WAVE':
						raise Exception('invalid file type (WAV)')
					f.seek(0)
					payload = remove_wav_header(f.read())
				else:
					payload = f.read()

			except Exception as e:
				print('%s : skipped, %s' % (file, e))
				continue
			finally:
				f.close()

		if len(payload):
			table += pack(lib_table_t, nameInTable.encode('cp1251'), offset, len(payload))
			data += payload
			offset += len(payload)
			fileCount += 1
			print('%s : ok' % file)

	# summary
	print('Lib: %s' % libfile)
	print('Type: %s' % magic_to_str(magic))
	print('Version: %i' % version)
	print('Total files: %i' % fileCount)
	print('Raw mode:', raw)
	print('Truncate filename:', srtict)

	# build header + data + index table
	with open(libfile, 'wb') as o:
		try:
			if fileCount > 0:
				offsetTable = calcsize(lib_header_t) + len(data)
			else:
				offsetTable = 0
			o.write(pack(lib_header_t, magic, fileCount, version, offsetTable) + data + table)
		except Exception as e:
			parser.error(e)
		finally:
			o.close()
	print('Done')

# arg checks


def file_type(path):
	if not isfile(path):
		raise argparse.ArgumentTypeError('the file does not exist')
	return path


def magic_type(magic):
	if str_to_magic(magic) == 0:
		raise argparse.ArgumentTypeError('invalid choice: %r (choose from bmp_v1, bmp_v2, anm_v1, anm_v2, anm_v3, wav_v1 or wav_v2)' % magic)
	return magic


def version_type(ver):
	value = int(ver)
	if value < 0 or value > 1:
		raise argparse.ArgumentTypeError('invalid choice: %i (choose from range [0,1])' % value)
	return value


# main
parser = argparse.ArgumentParser(prog='libex', description='MediaHouse libs resource unpacker/packer')
subparsers = parser.add_subparsers(title='commands', help='available utils', dest='command')

parser_upck = subparsers.add_parser('unpack', help='unpack lib file')
parser_upck.add_argument('-i', '--input', action='store', type=file_type, required=True,
					help='source lib file')
parser_upck.add_argument('-o', '--output', action='store', type=str, required=True,
					help='output directory to extract')
parser_upck.add_argument('-r', '--raw', action='store', type=bool, default=False,
					help='extract data without conversion (default: false)')

parser_pck = subparsers.add_parser('pack', help='pack list of files to lib file')
parser_pck.add_argument('-i', '--input', action='store', type=str, required=True,
					help='directory path to pack')
parser_pck.add_argument('-o', '--output', action='store', type=str, required=True,
					help='output lib file')
parser_pck.add_argument('-t', '--type', action='store', type=magic_type, required=True,
					help='out lib type: bmp_v1, bmp_v2, anm_v1, anm_v2, anm_v3, wav_v1 or wav_v2')
parser_pck.add_argument('-v', '--version', action='store', type=version_type, default=1,
					help='version of archive [0-1] (default: 1)')
parser_pck.add_argument('-r', '--raw', action='store_true', default=False,
					help='pack data without conversion (default: false)')
parser_pck.add_argument('-s', '--strict', action='store_true', default=True,
					help='Truncate filenames automatically')

args = parser.parse_args()

if args.command == 'unpack':
	unpack_lib(args.input, args.output, args.raw)
elif args.command == 'pack':
	pack_lib(args.type, args.version, args.input, args.output, args.raw, args.strict)
else:
	parser.print_usage()
	exit(1)