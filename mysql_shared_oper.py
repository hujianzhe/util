#coding=utf-8
import getopt
import sys
import os
import MySQLdb
reload(sys)
sys.setdefaultencoding('utf8')

# 解析命令行
conn_shared_config_path = "db_connection_config"
conn_shared_key = None
conn_shared_cnt = None

db_name = None
db_shared_key = None
db_shared_cnt = None

table_name = None
table_shared_key = None
table_shared_cnt = None

sql_operator = None
sql_field = None
sql_where = None
sql_is_select = False

is_print_debug_info = False

opt_array = ["conn_shared=", "conn_shared_cnt=", "db_name=", "db_shared=", "db_shared_cnt=", "table_name=", "table_shared=", "table_shared_cnt=",
"select=", "update=", "insert=", "delete", "where=", "config_path=", "debug"]
try:
	opts, args = getopt.getopt(sys.argv[1:], "", opt_array)
	for pair in opts:
		if pair[0] == '--conn_shared':
			conn_shared_key = int(pair[1])
		elif pair[0] == '--conn_shared_cnt':
			conn_shared_cnt = int(pair[1])
		elif pair[0] == '--db_name':
			db_name = pair[1]
		elif pair[0] == '--db_shared':
			db_shared_key = int(pair[1])
		elif pair[0] == '--db_shared_cnt':
			db_shared_cnt = int(pair[1])
		elif pair[0] == '--table_name':
			table_name = pair[1]
		elif pair[0] == '--table_shared':
			table_shared_key = int(pair[1])
		elif pair[0] == '--table_shared_cnt':
			table_shared_cnt = int(pair[1])
		elif pair[0] == '--select':
			sql_operator = 'select'
			sql_field = pair[1]
			sql_is_select = True
		elif pair[0] == '--update':
			sql_operator = 'update'
			sql_field = pair[1]
		elif pair[0] == '--insert':
			sql_operator = 'insert'
			sql_field[0] = pair[1]
		elif pair[0] == '--delete':
			sql_operator = 'delete'
		elif pair[0] == '--where':
			sql_where = pair[1]
		elif pair[0] == '--config_path':
			conn_shared_config_path = pair[1]
		elif pair[0] == '--debug':
			is_print_debug_info = True

	if conn_shared_key == None:
		print '未指定连接分片,--conn_shared'
		exit(1)
	elif db_name == None:
		print '未指定db名,--db_name'
		exit(1)
	elif table_name == None:
		print '未指定table名,--table_name'
		exit(1)
	elif db_shared_key != None and db_shared_cnt == None:
		print '未指定db总数,--db_shared_cnt'
		exit(1)
	elif db_shared_key == None and db_shared_cnt != None:
		print '未指定db分片,--db_shared'
		exit(1)
	elif table_shared_key != None and table_shared_cnt == None:
		print '未指定table总数,--table_shared_cnt'
		exit(1)
	elif table_shared_key == None and table_shared_cnt != None:
		print '未指定table分片,--table_shared'
		exit(1)
	elif sql_operator == None:
		print '未指定SQL操作,--select/--insert/--update/--delete'
		exit(1)

except getopt.GetoptError:
	print '解析命令参数错误'
	exit(1)

print '-------------------------------------------------------'

# 读取连接分片配置
conn_shared_options = []
f = open(conn_shared_config_path)
for line in f:
	conn_shared_options.append(eval(line))
f.close()
if conn_shared_cnt == None:
	conn_shared_cnt = len(conn_shared_options)
elif conn_shared_cnt != len(conn_shared_options):
	print '配置的分片数和指定的分片数不同'
	exit(1)
if conn_shared_cnt == 0:
	print '连接配置数目为空'
	exit(1)

# 组合分片信息
conn_shared_index = conn_shared_key % conn_shared_cnt
conn_option = conn_shared_options[conn_shared_index]

db_shared_index = None
if db_shared_key != None:
	db_shared_index = db_shared_key % db_shared_cnt
	db_name = db_name + '_' + str(db_shared_index)

table_shared_index = None
if table_shared_key != None:
	table_shared_index = table_shared_key % table_shared_cnt
	table_name = table_name + '_' + str(table_shared_index)

# 拼接SQL语句
sql = ''
if sql_operator.lower() == 'select':
	if sql_field == None:
		print '未指定查询字段,--select'
		exit(1)
	if sql_where == None:
		print '未指定查询条件,--where'
		exit(1)

	sql = 'select ' + sql_field + ' from ' + table_name + ' where ' + sql_where

elif sql_operator.lower() == 'insert':
	if sql_field == None:
		print '未指定插入字段,--insert'
		exit(1)

	sql = 'insert into ' + table_name + ' ' + sql_field

elif sql_operator.lower() == 'update':
	if sql_field == None:
		print '未指定更新字段,--update'
		exit(1)
	if sql_where == None:
		print '未指定更新条件,--where'
		exit(1)

	sql = 'update ' + table_name + ' set ' + sql_field + ' where ' + sql_where

elif sql_operator.lower() == 'delete':
	if sql_where == None:
		print '未指定删除条件,--where'
		exit(1)

	sql = 'delete from' + table_name + ' where ' + sql_where

else:
	print '不合法的SQL操作 %s' % sql_operator
	exit(1)

# 打印分片信息
if is_print_debug_info:
	print '-------------------------------------------------------------'
	print '连接分片配置路径: %s' % conn_shared_config_path
	for conn_option_item in conn_shared_options:
		print conn_option_item
	print 'db_conn_shared_index = %d' %  conn_shared_index
	if db_shared_index != None:
		print 'db_shared_index = %d' %  db_shared_index
	if table_shared_index != None:
		print 'table_shared_index = %d' %  table_shared_index
	print conn_option, db_name, table_name
	print sql
	print '-------------------------------------------------------------'

# DB操作
db_conn_obj = MySQLdb.connect(host=conn_option['host'], port=conn_option['port'] ,user=conn_option['user'], passwd=conn_option['pwd'], db=db_name, charset='utf8')
db_conn_cursor = db_conn_obj.cursor()
if not db_conn_cursor:
	print 'DB连接失败'
	exit(1)

try:
	db_conn_cursor.execute(sql)
	if sql_is_select:
		result_rows = db_conn_cursor.fetchall()
		result_cols = db_conn_cursor.description
		for i in range(len(result_cols)):
			print result_cols[i][0],'	',	#不换行输出, note: python2 3 语法不兼容
		print ''
		print '---------------------------------------------'
		for i in range(len(result_rows)):
			for j in range(len(result_rows[i])):
				print result_rows[i][j],'	',
			print ''
	else:
		db_conn_obj.commit()

except Exception as e:
	print 'SQL执行失败', e
	if not sql_is_select:
		db_conn_obj.rollback()

db_conn_obj.close()
