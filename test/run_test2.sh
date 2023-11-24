for ((i=1 ; i<= $1; i++)); do
    printf "CAPA\n" $i | ncat -C localhost 1110
done