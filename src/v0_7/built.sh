TARGET=best2500i_ibrt
arg_arr=($@)
SPACE=' '
if [ "${DEBUGCMD}" == "" ]
then
DEBUGCMD=echo
fi

if [ "${1}" == "all" ]
then
unset arg_arr[0]
arg_list=''
for arg in ${arg_arr[@]}
do
	arg_list=${arg_list}${SPACE}${arg}
done
for arg in d ai anc
do
	${DEBUGCMD} bash ${0} ${arg} ${arg_list}
done
exit 0
fi

if [ "${1}" != 'd' ]
then
	TARGET=${TARGET}_${1}
fi
if [ "${2}" == 'f' ]
then
	unset arg_arr[1]
	${DEBUGCMD} make T=${TARGET} allclean -j
fi
unset arg_arr[0]
arg_list=''
for arg in ${arg_arr[@]}
do
	arg_list=${arg_list}${SPACE}${arg}
done
${DEBUGCMD} make T=${TARGET} ${arg_list} -j
