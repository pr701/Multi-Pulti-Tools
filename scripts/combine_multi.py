import os
import errno

from shutil import copyfile
from os import listdir
from os.path import isfile, isdir, join

target = 'multi'

sources = {
	'Мульти-Пульти', 'Новые Бременские',
	'Незнайка и все-все-все', 'Мои любимые герои'
}

libs = {
	'anm', 'anm_s', 'fon', 'fon_s',
	'stat', 'stat_s', 'wav_ef', 'wav_mus'
}


def createDir(dir):
	try:
		if not isdir(dir):
			os.makedirs(dir)
	except OSError as e:
		if e.errno != errno.EEXIST:
			raise


def getNewFilename(src, dst_list):
	src_name, src_ext = os.path.splitext(src)
	index = 0
	while True:
		if index > 0:
			final_name = src_name + ' - ' + str(index)
		else:
			final_name = src_name
		final_name += src_ext
		if not any(final_name.lower() == item.lower() for item in dst_list):
			break
		index += 1
	return final_name


# not optimize compare
def CmpBinary(file1, file2):
	result = False
	if isfile(file1) and isfile(file2):
		f1 = open(file1, 'rb')
		f2 = open(file2, 'rb')
		try:
			d1 = f1.read()
			d2 = f2.read()
			if len(d1) != len(d2):
				raise
			for i in range(0, len(d1)):
				if d1[i] != d2[i]:
					raise
			result = True
		except Exception:
			pass
		finally:
			f1.close()
			f2.close()
	return result


for source in sources:
	for lib in libs:
		dst_path = os.path.join(target, lib)
		src_path = os.path.join(source, lib)
		createDir(dst_path)
		print('Copy lib data...')
		print('Src %s' % src_path)
		print('Dst %s' % dst_path)
		dst_list = [f for f in listdir(dst_path) if isfile(join(dst_path, f))]
		src_list = [f for f in listdir(src_path) if isfile(join(src_path, f))]
		for src in src_list:
			if CmpBinary(os.path.join(src_path, src), os.path.join(dst_path, src)):
				continue
			name = getNewFilename(src, dst_list)
			print(name)
			copyfile(os.path.join(src_path, src), os.path.join(dst_path, name))
			dst_list.append(name)
