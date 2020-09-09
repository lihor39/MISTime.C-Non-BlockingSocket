# notCrashed= true
while $notCrashed; do
    if ./MIS -p 2 randLocalGraph_J_5_10; then
    	echo "Completed successfully"
    	exit 0
        # notCrashed= false
    else
	until ./MIS -p 2 -t 2 randLocalGraph_J_5_10; do
	    echo "Server 'MIS' crashed with exit code $?.  Respawning.." >&2
	    sleep 1
	exit 0
	done 	
    fi
done
