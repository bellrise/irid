#!/bin/sh
# Run the assembler testing suite.

sources=$(find tests -name '*.i')
failed=0

for source in $sources; do
    name=$(echo $(basename "$source") | cut -d'.' -f1)

    irid-as -o build/result.bin tests/$name.i
    [ ! -f build/result.bin ] && {
        echo "[-] Failed $name"
        failed=$(($failed + 1))
        continue
    }

    r1=$(md5sum build/result.bin | cut -d' ' -f1)
    r2=$(md5sum tests/$name.bin | cut -d' ' -f1)

    [ "$r1" = "$r2" ] && {
        echo "[+] Passed $name"
    } || {
        echo "[-] Failed $name"
        failed=$(($failed + 1))
    }
done

[ $failed = 0 ] || {
    echo -e "\033[31m[-] Failed $failed test(s)\033[0m"
}
