db=$TGFDB
echo "sqlite3 -column -header $db \"select * from lightning order by utc asc;\""
echo ''
sqlite3 -column -header $db "select * from lightning order by utc asc;"
echo '# Rows:'
sqlite3 -noheader $db "select count(*) from lightning;"
echo "DONE"