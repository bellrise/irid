#!/bin/sh
# Run the assembler testing suite.

sources=$(find tests -name '*.i')
total=0
failed=0

for source in $sources; do
    name=$(echo $(basename "$source") | cut -d'.' -f1)

    total=$(($total + 1))
    irid-as -o build/result.bin tests/$name.i
    [ ! -f build/result.bin ] && {
        echo "[-] Failed $name"
        failed=$(($failed + 1))
        continue
    }

    r1=$(md5sum build/result.bin | cut -d' ' -f1)
    r2=$(md5sum tests/$name.bin | cut -d' ' -f1)

    [ "$r1" = "$r2" ] && {
        echo -e "[\033[32m+\033[0m] Passed $name"
    } || {
        echo -e "[\033[31m-\033[0m] Failed $name"
        failed=$(($failed + 1))
    }
done

[ $failed = 0 ] || {
    echo -e "\033[31m[-] Failed $failed out of $total test(s)\033[0m"
}
