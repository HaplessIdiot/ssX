REM 
/* OS/2 generate header files */
/* $XFree86$ */
cat psclrops.h psctrlops.h psctxtops.h psdataops.h psfontops.h psgsttops.h psioops.h psmathops.h psmtrxops.h psmiscops.h pspntops.h pspathops.h pssysops.h pswinops.h psopstack.h psXops.h psl2ops.h >.ph
sed -e "/^$$/D" -e "/#/D" -e "/^\//D" -e "/^   gener/D" -e "/^.\//D" .ph | sort >.sort
awk "/;/ {print;printf(\"\n\");}" .sort >.ttt
cat psclrops.ah psctrlops.ah psctxtops.ah psdataops.ah psfontops.ah psgsttops.ah psioops.ah psmathops.ah psmtrxops.ah psmiscops.ah pspntops.ah pspathops.ah pssysops.ah pswinops.ah psopstack.ah psXops.ah psl2ops.ah >.pah
sed -e "/^$$/D" -e "/#/D" -e "/^\//D" -e "/^   gener/D" -e "/^.\//D" .pah | sort >.sort
awk "/;/ {print;printf(\"\n\");}" .sort >.attt
cat psname.txt header.txt psifdef.txt .ttt else.txt .attt psendif.txt > psops.h.os2
rm .ph .pah .sort .ttt .attt

cat dpsclrops.h dpsctrlops.h dpsctxtops.h dpsdataops.h dpsfontops.h dpsgsttops.h dpsioops.h dpsmathops.h dpsmtrxops.h dpsmiscops.h dpspntops.h dpspathops.h dpssysops.h dpswinops.h dpsopstack.h dpsXops.h dpsl2ops.h >.ph
sed -e "/^$$/D" -e "/#/D" -e "/^\//D" -e "/^   gener/D" -e "/^.\//D" .ph | sort >.sort
awk "/;/ {print;printf(\"\n\");}" .sort >.ttt
cat dpsclrops.ah dpsctrlops.ah dpsctxtops.ah dpsdataops.ah dpsfontops.ah dpsgsttops.ah dpsioops.ah dpsmathops.ah dpsmtrxops.ah dpsmiscops.ah dpspntops.ah dpspathops.ah dpssysops.ah dpswinops.ah dpsopstack.ah dpsXops.ah dpsl2ops.ah >.pah
sed -e "/^$$/D" -e "/#/D" -e "/^\//D" -e "/^   gener/D" -e "/^.\//D" .pah | sort >.sort
awk "/;/ {print;printf(\"\n\");}" .sort >.attt
cat dpsname.txt header.txt dpsifdef.txt .ttt else.txt .attt dpsendif.txt > dpsops.h.os2
rm .ph .pah .sort .ttt .attt
