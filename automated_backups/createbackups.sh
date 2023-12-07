# I have used an argument in find command which is -not -path "*/.*", this helps the script to avoid looking into folder who's name starts with a .
# folders with a dot in the start of their name are meant to be ignored, in a general sense

# record number of complete and incremental backups
cb_count=0
ib_count=0

# 2 minutes delay
delay=120

# create directory structure
if [ ! -e $HOME ]; then
    mkdir $HOME
fi

if [ ! -e $HOME/backup ]; then
    mkdir $HOME/backup
else
    rm -r $HOME/backup
    mkdir $HOME/backup
fi

if [ ! -e $HOME/backup/cb ]; then
    mkdir $HOME/backup/cb
fi

if [ ! -e $HOME/backup/ib ]; then
    mkdir $HOME/backup/ib
fi

# recreate backup file
if [ -e backup.log ]; then
    rm backup.log
fi
touch backup.log

# perform tasks in an infinite loop
while true; do
    # Step 1: Complete backup
    cb_tar_fname=${HOME}/backup/cb/cb0$(printf "%04d" $cb_count).tar

    # find all c and txt files in home directory and pipe the output to tar command
    # -T is used to specify the filename to tar, -T - tells tar to use stdin as input file
    find $HOME -type f \( -name "*.pdf" -o -name "*.txt" \) -not -path "*/.*" | tar -T - -Pcf "$cb_tar_fname"

    # update backup file
    echo `date` $cb_tar_fname  was created >> backup.log
    echo  >> backup.log

    # record backup timestamp and increment count
    cb_timestamp=`date "+%Y-%m-%d %H:%M:%S"`
    cb_count=$((cb_count+1))

    echo Complete backup \#$cb_count done

    # wait for 2 minutes after complete backup
    sleep $delay
    
    # Step 2, 3, 4: Incremental backups
    # as step 3, 4, 5 are same except that they check for files modified since previous backups, we can create a loop
    # the loop will run thrice for each incremental backup step, refer to previous steps timestamp and create a tar file
    ib_step=1
    while (( ib_step <=3 ))
    do
        ib_tar_fname=${HOME}/backup/ib/ib1$(printf "%04d" $ib_count).tar

        # find new modified files
        if (( ib_step == 1 )); then
            ib_c_txt=$(find $HOME -type f \( -name "*.pdf" -o -name "*.txt" \) -not -path "*/.*" -newermt "$cb_timestamp")
        else
            ib_c_txt=$(find $HOME -type f \( -name "*.pdf" -o -name "*.txt" \) -not -path "*/.*" -newermt "$ib_timestamp")
        fi

        # check if modified files result is not empty
        if [ -n "$ib_c_txt" ]; then
            # push modified file names to stdin of tar command via echo command
            echo -n "$ib_c_txt" | tar -T - -Pcf "$ib_tar_fname"

            echo `date` $ib_tar_fname  was created >> backup.log
            echo  >> backup.log

            # update incremental backup time and count
            ib_timestamp=`date "+%Y-%m-%d %H:%M:%S"`
            ib_count=$((ib_count+1))

            echo Incremental backup \#$ib_count done, modified files: $ib_c_txt
        else
            echo "`date` No changes-Incremental backup was not created" >> backup.log
            echo >> backup.log

            echo Incremental backup, nothing modified
        fi
        
        sleep $delay

        ((ib_step++))
    done

done
