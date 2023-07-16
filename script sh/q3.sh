nflows=(1 2 3 4 5 6 7 8) #creo un vettore 
nsSeed=(41 55 148) #seed for ns-3

confidencePath=my-simulations/confidence-int/intconfidence.tcl #for calculating the confidence interval
#rm *.txt
#rm *.confint
#rm -r results


for n in ${!nflows[@]}; do #avvio un ciclo FOR per tutti i valori del vettore

echo "Launching simulation with "${nflows[$n]}" WiFi STA" # scritta video 

AP=2 #ID of the AP

    for run in {0..2}    

    do
	echo "Run "$run" seed "${nsSeed[$run]}
	./ns3 run "esameq3 --flows=${nflows[$n]} --seed=${nsSeed[$run]}" ## launch simulation

    done #end for run

    for file in *.txt #take as input all the txt files in the folder
     do
	echo "Creating "$file "with confidence interval"

	$confidencePath $file 0.05 ${nflows[$n]}
        cat $file.confint>>$file-final.out #saving final results
     done
#rm *.confint
#rm *.txt
done #end for STA


mkdir results
mv *.out results



