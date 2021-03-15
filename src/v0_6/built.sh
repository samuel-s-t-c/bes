TARGET=best2500i_ibrt
if [ ${2} ]
then
	TARGET=${TARGET}_${2}
fi
if [ ! ${1} ]
then
	make T=${TARGET} -j
else if [ ${1} == 'f' ]
then
	make T=${TARGET} allclean -j
	make T=${TARGET} -j
else
	make T=${TARGET}_${1} -j
fi
fi
