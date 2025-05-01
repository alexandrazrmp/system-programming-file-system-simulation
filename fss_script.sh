#!/bin/bash

#check input
if [[ $# -ne 4 || $1 != "-p" || $3 != "-c" ]]; then
    echo "Please insert in the form ./fss_script -p <path> -c <command>"
    exit 1
fi

path="$2"
command="$4"

listAll() {
    if [[ ! -f $path ]]; then
        echo "Invalid Path to log_file."
        exit 1
    fi

    #grep to only get lines that start with "Added directory:" or "Syncing directory:"
    grep "Added directory:|Syncing directory:" "$path" | while read -r line; do

        if [[ "$line" == *"Added directory:"* ]]; then
            pair="${line#*Added directory: }"  #remove Added directory:
        else
            pair="${line#*Syncing directory: }"  #remove Syncing directory:
        fi

        #get timestamp
        timestamp="${line%%]*}" #get the timestamp that ends with ]
        timestamp="${timestamp#[}"  #remove [

        #get src and dst pair
        pair="${line#*Added directory: }"
        src="${pair%% ->*}" #get the source dir that ends with ->
        dst="${pair#*-> }"  #remove the src dir and -> from the pair to get the destination dir

        if [[ "$dst" == "(null)" ]]; then
            dst="null (sorry, something wrong with fss_manager log_file)"
        fi

        echo "$src -> $dst [Last Sync: $timestamp]"
    done
}


listMonitored() {
    echo "not implemented"
}

listStopped() {
    echo "not implemented"
}


#deletion of log-file or directory
purge() {
    if [[ -f $path ]]; then     #case of file
        echo "Purging a log-file:"
        echo "Deleting $path..."
        rm -f "$path"
        echo "Purge complete."
    elif [[ -d $path ]]; then   #case of directory
        echo "Purging backup directory:"
        echo "Deleting $path..."
        rm -rf "$path"
        echo "Purge complete."
    else
        echo "Invalid Directory/File."
        exit 1
    fi
}


#Switch command
case "$command" in
    listAll)
        listAll
        ;;
    listMonitored)
        listMonitored
        ;;
    listStopped)
        listStopped
        ;;
    purge)
        purge
        ;;
    *)
        echo "Invalid Input."
        exit 1
        ;;
esac
