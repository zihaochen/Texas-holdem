import os
import sys
import datetime
import xlsxwriter
from xlsxwriter.utility import xl_rowcol_to_cell

pwd = os.getcwd()

def get_date_from_log(file):
	if len(file) != 27 or file[:4] != "log_" or file[-4:] != ".log":
		return False
	
	return datetime.datetime.strptime(file, "log_%Y-%m-%d-%H-%M-%S.log")

def get_name_value_pair(line, value_index, player_index = None, sentinel = None):
	line = line.split(' ')
	if player_index != None:
		try:
			player_index_end = line[player_index:].index(sentinel)
			name = ' '.join(line[player_index : player_index + player_index_end])
		except ValueError:
			name = None
	else:
		name = None
	value = line[value_index]
	return name, value	

def get_player_count(fin):
	fin.readline()
	fin.readline()
	fin.readline()
	line = fin.readline()
	return eval(line[line.rfind('=')+2:])

def get_total_games(fin):
	line = fin.readline()
	while len(line) < 12 or line[:12] != "[FINAL STAT]":
		line = fin.readline()
	return eval(line[line.rfind(':')+2:])

def get_original_name(name):
	p = name.rfind('_')
	if p != -1:
		p = name.rfind('_', 0, p)
	if p!= -1:
		return name[:p]
	else:
		return name

def add_item(stat_map, name, item, time):
	name = get_original_name(name)
	if name not in stat_map:
		stat_map[name] = {time : item}
	else:
		stat_map[name][time] = item

def get_stats_of_one_item(fin, stat_map, skip_cnt, player_cnt, value_index, player_index, sentinel, time, do_eval = True):
	while skip_cnt > 0:
		skip_cnt -= 1
		fin.readline()

	while player_cnt > 0:
		player_cnt -= 1
		line = fin.readline()
		name, value = get_name_value_pair(line, value_index, player_index, sentinel)
		if do_eval:
			value = eval(value)
		add_item(stat_map, name, value, time)

row_cnt = 0
def write_per_game_stats(worksheet, title, data, span, stat_func_title):
	global row_cnt
	row = row_cnt
	row_cnt = row_cnt + 1
	worksheet.write(row, 0, title)

	for i, item in enumerate(stat_func_title):
		worksheet.merge_range(row, i*span+1, row, (i+1)*span, item)

	for i, item in enumerate(data):
		worksheet.merge_range(row, (i+len(stat_func_title))*span+1, row, (i+len(stat_func_title)+1)*span, item)


def write_per_player_stats_title(worksheet, title, num_of_col):
	global row_cnt
	row = row_cnt
	row_cnt += 1

	worksheet.write(row, 0, "name")
	col = 0
	for i in xrange(num_of_col):
		for item in title:
			col += 1
			worksheet.write(row, col, item)

def write_per_player_stats(worksheet, player_name, player_maps, Time, stat_func):
	global row_cnt
	row = row_cnt
	row_cnt += 1

	worksheet.write(row, 0, player_name)
	col = 0
	player_map = map(lambda whole_map : whole_map[player_name], player_maps)

	range_str = "%s:%s" % (xl_rowcol_to_cell(row, len(stat_func) * len(player_map) + 1), xl_rowcol_to_cell(row, len(player_map) * (len(stat_func) + len(Time))))
	for func in stat_func:
		for i in xrange(len(player_map)):
			col += 1
			worksheet.write_formula(row, col, 
				"{=%s(%s * (MOD(COLUMN(%s),%d)=%d))}" % (func, range_str, range_str, len(player_map), i))

	for time in Time:
		for item in player_map:
			col += 1
			if time in item:
				worksheet.write(row, col, item[time])
			else:
				worksheet.write(row, col, None)
	
			

def set_column_width(worksheet, first, next, num_of_col):
	worksheet.set_column(0, 0, first)
	col = 0
	for i in xrange(num_of_col):
		for w in next:
			col += 1
			worksheet.set_column(col, col, w)

def set_column_format(workbook, worksheet, num_item, num_time, num_stat_func, column_width):
	average_chips_format = workbook.add_format()
	average_chips_format.set_num_format("0")
	average_chips_col = 3
	worksheet.set_column(average_chips_col, average_chips_col, column_width[0], average_chips_format)

	average_survival_time_format = workbook.add_format()
	average_survival_time_format.set_num_format("0.0")
	average_survival_time_col = average_chips_col + 1
	worksheet.set_column(average_survival_time_col, average_survival_time_col, column_width[1], average_survival_time_format) 

def get_stats_from_dir(dir, workbook, worksheet):
	print "Entering", dir

	dir += '/'
	os.chdir(dir)
	files = os.listdir("./")

	Time = []
	PlayerCnt = []
	TotalGames = []
	chips_map = {}
	survival_time_map = {}
	for file in files:
		time = get_date_from_log(file)
		if time == False:
			continue

		print "Processing", file
		fin = open(file, "r")
		
		player_count = get_player_count(fin)
		total_games = get_total_games(fin)
		
		Time.append(time)
		PlayerCnt.append(player_count)
		TotalGames.append(total_games)

		get_stats_of_one_item(fin, chips_map, 1, player_count, -2, 3, 'has', time)	
		get_stats_of_one_item(fin, survival_time_map, 1, player_count, -2, 3, 'has', time)

		fin.close()
	
	
	global row_cnt
	row_cnt = 0

	stat_func_title = ["Sum", "Average"]
	stat_func = ["SUM", "AVERAGE"]
	player_maps = [chips_map, survival_time_map]
	column_span = max(1, len(player_maps))
	column_width = [8, 12] # for chips and survival time
	set_column_width(worksheet, 12, column_width, len(Time) + len(stat_func_title))
	set_column_format(workbook, worksheet, len(player_maps), len(Time), len(stat_func_title), column_width)
	worksheet.freeze_panes(4, 1)
	
	write_per_game_stats(worksheet, "Time", Time, column_span, stat_func_title)
	write_per_game_stats(worksheet, "#Player", PlayerCnt, column_span, [''] * len(stat_func_title))
	write_per_game_stats(worksheet, "#Game", TotalGames, column_span, [''] * len(stat_func_title))

	write_per_player_stats_title(worksheet, ["Chips", "Survival time"], len(Time) + len(stat_func_title))
	for name in chips_map.viewkeys():
		write_per_player_stats(worksheet, name, player_maps, Time, stat_func)

	os.chdir(pwd)

argc = len(sys.argv)
if argc < 3:
	print "Usage: <output_file_name> <directory> [...]"
	sys.exit(0)

workbook = xlsxwriter.Workbook(sys.argv[1], 
							   {"default_date_format": "mmmm d yyyy hh:mm:ss"})

for dir in sys.argv[2:]:
	get_stats_from_dir(dir, workbook, workbook.add_worksheet(dir.replace('/', '_').replace('\\','_')))

workbook.close()

