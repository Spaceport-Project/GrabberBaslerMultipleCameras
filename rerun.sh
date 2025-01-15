#!/bin/bash

# Maximum number of retry attempts
MAX_RETRIES=5
# Delay between retries in seconds
RETRY_DELAY=10
# Check if a program name is provided
if [ $# -eq 0 ]; then
    echo "Usage: $0 <program_name> [program_arguments]"
    exit 1
fi

# Store the program name and its arguments
PROGRAM="$1"
shift
ARGUMENTS="$@"

attempt=1
while [ $attempt -le $MAX_RETRIES ]; do
    echo "Attempt $attempt of $MAX_RETRIES"
    
    # Run the program with its arguments
    $PROGRAM $ARGUMENTS
    exit_code=$?
    
    # Check if program executed successfully
    if [ $exit_code -eq 0 ]; then
        echo "Program completed successfully"
        exit 0
    else
        echo "Program failed with exit code $exit_code"
        
        # Check if we should retry
        if [ $attempt -lt $MAX_RETRIES ]; then
            echo "Retrying in $RETRY_DELAY seconds..."
            sleep $RETRY_DELAY
        else
            echo "Maximum retry attempts reached. Giving up."
            exit $exit_code
        fi
    fi
    
    ((attempt++))
done
