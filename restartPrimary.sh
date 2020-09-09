# notCrashed= true
while $notCrashed; do
    if ./MIS -p 1 randLocalGraph_J_5_10; then
    	echo "Completed successfully"
    	exit 0
        # notCrashed= false
    else
	until ./MIS -p 1 -t 2 randLocalGraph_J_5_10; do
	    echo "Server 'MIS' crashed with exit code $?.  Respawning.." >&2
	    sleep 1
	done 	
    fi
done
