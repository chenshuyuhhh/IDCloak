#!/bin/bash

#run end2end experiments

params=(
    "1700 111"
)

times=3
type=$1
mode=$2
specific_param=$3
specific_n=$4

echo $type
echo $mode

case $type in
    "fa")
        path="./exp/datasets/fa"
        python3 scripts/settype.py 2
        ;;
    "psi")
        path="./exp/datasets/psi"
        python3 ./scripts/settype.py 1
        ;;
    *)
        echo "Invalid type. Use 'psi' or 'fa'."
        exit 1
        ;;
esac

echo $path

run_experiment() {
    local srow=$1
    local scol=$2
    local n=$3

    echo "Update: nParties=$n, srow=$srow, scol=$scol"
    if [ "$type" == "fs" ]; then
        total=$scol
        part=$((total / n))
        a=()
        sum=0
        
        for ((i=0; i<n-1; i++)); do
            a+=($part)
            sum=$((sum + part))
        done
        a+=($((total - sum)))
        
        a_str=$(IFS=,; echo "${a[*]}")
        python3 ./scripts/setf.py "$n" "$srow" "$a_str"
    elif [ "$type" == "psi" ]; then
        python3 ./scripts/setpsi.py "$n" "$srow"
    elif [ "$type" == "shuffle" ]; then
        python3 ./scripts/setshuffle.py --nParties "$n" --srow "$srow" --scol "$scol"
    fi

    for ((j = 0; j < times; j++)); do # run three times
        for ((i = 0; i < n; i++)); do
            command="./build/IDCloak -p $i >> $path/$srow-$scol-$n-$i.txt &"
            eval $command
            echo $command
            # echo $path/$srow-$scol-$n-$i.txt
            sleep 2
        done
        wait
        echo $path/$srow-$scol-$n
        sleep 5
    done
    wait
    sleep 5
}

if [ -n "$specific_param" ] && [ -n "$specific_n" ]; then
    IFS=' ' read -r srow scol <<< "$specific_param"
    echo $srow $scol $specific_n
    run_experiment "$srow" "$scol" "$specific_n"
else
    for param in "${params[@]}"; do
        IFS=' ' read -r srow scol <<< "$param"
        for n in {2..6}; do
            run_experiment "$srow" "$scol" "$n"
        done
    done
fi