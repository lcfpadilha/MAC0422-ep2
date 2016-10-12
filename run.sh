#PEQUENA PISTA

for i in `seq 1 30`; do
    /usr/bin/time -v ./ep2 250 5 u 2>&1 > /dev/null | grep "Maximum resident" | cut -d':' -f 2 >> results/p-p.txt
done

for i in `seq 1 30`; do
    /usr/bin/time -v ./ep2 250 30 u 2>&1 > /dev/null | grep "Maximum resident" | cut -d':' -f 2 >> results/p-m.txt
done

for i in `seq 1 30`; do
    /usr/bin/time -v ./ep2 250 50 u 2>&1 > /dev/null | grep "Maximum resident" | cut -d':' -f 2 >> results/p-g.txt
done

#MEDIA PISTA


for i in `seq 1 30`; do
    /usr/bin/time -v ./ep2 750 5 u 2>&1 > /dev/null | grep "Maximum resident" | cut -d':' -f 2 >> results/m-p.txt
done

for i in `seq 1 30`; do
    /usr/bin/time -v ./ep2 750 30 u 2>&1 > /dev/null | grep "Maximum resident" | cut -d':' -f 2 >> results/m-m.txt
done

for i in `seq 1 30`; do
    /usr/bin/time -v ./ep2 750 50 u 2>&1 > /dev/null | grep "Maximum resident" | cut -d':' -f 2 >> results/m-g.txt
done

#GRANDE PISTA

for i in `seq 1 30`; do
    /usr/bin/time -v ./ep2 1500 5 u 2>&1 > /dev/null | grep "Maximum resident" | cut -d':' -f 2 >> results/g-p.txt
done

for i in `seq 1 30`; do
    /usr/bin/time -v ./ep2 1500 30 u 2>&1 > /dev/null | grep "Maximum resident" | cut -d':' -f 2 >> results/g-m.txt
done

for i in `seq 1 30`; do
    /usr/bin/time -v ./ep2 1500 50 u 2>&1 > /dev/null | grep "Maximum resident" | cut -d':' -f 2 >> results/g-g.txt
done
