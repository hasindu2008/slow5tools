for i in $(seq 1 $3); do 
    if ! diff <(head "-$i" $1) <(head "-$i" $2); then 
        echo $i
        diff <(xxd <(head "-$i" $1)) <(xxd <(head "-$i" $2))
        break
    fi
done
