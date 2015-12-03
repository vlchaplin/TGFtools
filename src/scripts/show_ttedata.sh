tbl="tte"
cmd="select type, utc_start, utc_stop, tmin, tmax, count(*) as \"# Files\" from $tbl group by utc_start order by utc_start asc;"
db=`tgftools_config --datadb`
echo "sqlite3 -column -header $db \"$cmd;\""
echo ''
sqlite3 -column -header $db "$cmd"
echo '# Files:'
sqlite3 -noheader $db "select count(*) from $tbl;"
echo "DONE"