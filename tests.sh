#!/bin/bash

#Made by Ron Even

VERSION=1.0

TMP_FILE="./.tmpfile"

DEBUG=0
if [ "$1" == "-d" ]; then
        DEBUG=1
fi

COUNTER=0
FAILED_TESTS=""

TEST_DIR=tests_ex4

print_single_invalid_output()
{
    echo -e "$1\n"
    if [ $DEBUG -eq 1 ]; then
            echo -e "Expected as hex:"
            echo -e "$1" | hexdump -C
            printf "\n"
    fi
}

print_actual_output()
{
    echo -e "Got:\n$1\n"
    if [ $DEBUG -eq 1 ]; then
            echo -e "Got as hex:"
            echo -e "$1" | hexdump -C
            echo -e "\n"
    fi
}

print_error_header_for_command()
{
    echo -e "\t\t❌ FAILED"
    echo -e "$1\n"
}

print_success_header()
{
    echo -e "\t\t✅ PASSED"
}

validate() {
        test_file="$1"
        arguments="$2"
        COUNTER=$((COUNTER + 1))
        echo "*****************************************"
        printf "$COUNTER  "

        make -f Makefile_test TEST_FILE="$test_file" > /dev/null 2>&1
        if [ $? -ne 0 ]
        then
            print_error_header_for_command "$test_file"
            echo -e "Build failed. To test why, you can build manually with:"
            echo -e "cd tests_ex4 ; make -f Makefile_test TEST_FILE='$test_file' ; cd -"
        fi
        #eval "./a.out $arguments" > $TMP_FILE 2>&1
        eval "timeout 20 ./a.out $arguments" > $TMP_FILE 2>&1
        error=$?
        result=$(cat $TMP_FILE)
        rm -f $TMP_FILE

        # Ignore first arguments
        shift
        shift

        matched=0
        general_error=0

        for arg in "$@"
        do
            expected=`printf "$arg"`

            if [ "$error" -ne 0 ]
            then
                   print_error_header_for_command "TEST_FILE=$test_file ; ARGS=\"$arguments\""
                   if [ "$error" -eq 124 ]
                   then
                      echo -e "Test timeouted! Make sure you have no deadlocks and your logic works properly!"
                    else
                      echo -e "Return code was $error and not 0! Make sure the program doesn't crash!"
                      echo "OUTPUT:"
                      echo "$result"
                   fi

                   FAILED_TESTS+="$COUNTER,"
                   general_error=1
                   break
            fi

            if [ "$result" == "$expected" ]
            then
                    print_success_header
                    matched=1
            fi
        done

        # If we reached here and matched=0 it means no matches were found. Print details
        if [ $matched -eq 0 -a $general_error -ne 1 ]
        then
            print_error_header_for_command "TEST_FILE=\$test_file ; ARGS=\"\$arguments\""
            print_actual_output "$result"

            if [ $# -gt 1 ]
            then
                echo -e "Expected (one of the $# following outputs):\n"
                for arg in "$@"
                do
                    expected=`printf "%s" "$arg"`

                    print_single_invalid_output "$expected"

                    if [ "$arg" != "${@: -1}" ]
                    then
                        echo -e "----- OR -----\n"
                    fi
                done
            else
                echo -e "Expected:\n"
                print_single_invalid_output "$expected"
            fi

            FAILED_TESTS+="$COUNTER,"
        fi
}

validate_test_files() {
    REQUIRED_FILES=$TEST_DIR
    NOT_FOUND_REQUIRED_FILES=0
    for required in $REQUIRED_FILES
    do
        found=$(ls | grep -wc "$required")
        if [ $found -eq 0 ]
        then
            echo "❌ couldn't find $required"
            NOT_FOUND_REQUIRED_FILES=$[NOT_FOUND_REQUIRED_FILES + 1]
        fi
    done

    if [ $NOT_FOUND_REQUIRED_FILES -ne 0 ]
    then
        echo "$NOT_FOUND_REQUIRED_FILES files/directories couldn't be found in the tests directory (`pwd`)"
        echo "Please make sure they exist and run tests.sh again"
        exit 1
    fi
}

declare_step() {
    echo "Testing $1..."
}

declare_stage() {
    echo "           Testing $1             "
}

validate_test_files

repeat_word() {
    yes "$1" | head -$2 | xargs echo | sed 's/ /\\n/g'
}

if [ -f "./osqueue.c" -a -f "./osqueue.h" -a -f "./threadPool.c" -a -f "./threadPool.h" ]
then
        cd $TEST_DIR

        declare_step "Sanity test from submit"
        validate "test_sanity.c" ""                                       "$(repeat_word hello 5)"

        declare_step "Simple functionality"
        validate "test_context.c" "5 5 1"                                 "$(repeat_word hello6 5)"

        declare_step "shouldWaitForTasks = 1"
        validate "test_simple.c" "5 10 1"                                 "$(repeat_word hello 10)"
        validate "test_simple.c" "10 5 1"                                 "$(repeat_word hello 5)"
        validate "test_simple.c" "10 10 1"                                "$(repeat_word hello 10)"
        validate "test_sleep.c"  "5 10 1"                                 "$(repeat_word hello 10)"
        validate "test_sleep.c"  "10 5 1"                                 "$(repeat_word hello 5)"
        validate "test_sleep.c"  "10 10 1"                                "$(repeat_word hello 10)"

        declare_step "shouldWaitForTasks = 0"
        validate "test_sleep.c"  "5 10 0"                                 "$(repeat_word hello 5)"
        validate "test_sleep.c"  "10 5 0"                                 "$(repeat_word hello 5)"
        validate "test_sleep.c"  "10 10 0"                                "$(repeat_word hello 10)"

        declare_step "single thread multiple tasks"
        validate "test_simple.c" "1 20 1"                                 "$(repeat_word hello 20)"
        validate "test_sleep.c"  "1 20 0"                                 "$(repeat_word hello 1)"

        declare_step "single task multiple threads"
        validate "test_simple.c" "20 1 1"                                 "$(repeat_word hello 1)"
        validate "test_sleep.c"  "20 1 0"                                 "$(repeat_word hello 1)"

        declare_step "single thread single task"
        validate "test_simple.c" "1 1 1"                                  "$(repeat_word hello 1)"
        validate "test_simple.c" "1 1 0"                                  "$(repeat_word hello 1)"

        declare_step "sleeps"
        validate "test_sleep_by_index.c" "5 10 1"                         "$(repeat_word hello 10)"
        validate "test_sleep_by_index.c" "10 10 1"                        "$(repeat_word hello 10)"
        validate "test_sleep_by_index.c" "15 10 1"                        "$(repeat_word hello 10)"

        validate "test_sleep_by_index.c" "1 10 0"                         "$(repeat_word hello 1)"
        validate "test_sleep_by_index.c" "5 10 0"                         "$(repeat_word hello 5)"
        validate "test_sleep_by_index.c" "10 10 0"                        "$(repeat_word hello 10)"

else
        echo "❌ Couldn't find $FILE file!"
        echo "Make sure the tests script is in the same directory as $FILE"
fi

echo ""
echo "************ 📝  SUMMARY  📝 ************"
failed_count=$(echo $FAILED_TESTS | tr -cd ',' | wc -c)
success_count=$((COUNTER-failed_count))
echo "           $success_count/$COUNTER tests passed!"
if [ $success_count -ne $COUNTER ]
then
    echo "Failed tests are:"
    FAILED_TESTS=${FAILED_TESTS%?}
    echo $FAILED_TESTS
    echo "NOTE: To build a test manually, run the following command sequence, and run a.out:"
    echo "TEST_FILE=<PUT TEST C FILE NAME HERE>"
    echo 'ARGS="<PUT TEST ARGUMENTS HERE>"'
    echo "cd tests_ex4 ; make -f Makefile_test TEST_FILE='$TEST_FILE' ; ./a.out $ARGS ; cd -"

    echo "NOTE: Expected and actual results look the same?"
    echo "Maybe there is a hidden character or an excess \n your code prints."
    echo "Run \"$0 -d\" to see the exact ASCII bytes of the expected and actual results"
fi

echo "*****************************************"


echo ""
echo "*****************************************"
echo "**  This script was made by Ron Even   **"
echo "**         script version: $VERSION         **"
echo "*****************************************"