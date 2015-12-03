tbl=$POS_TABLE

if [ "$tbl" == "" ]
then tbl="pos"
fi
cmd="select type, utc_start, utc_stop, tmin, tmax from $tbl order by utc_start asc;"
db=`tgftools_config --datadb`
echo "sqlite3 -column -header $db \"$cmd;\""
echo ''
sqlite3 -column -header $db "$cmd"
echo '# Rows:'
sqlite3 -noheader $db "select count(*) from $tbl;"
echo "DONE"