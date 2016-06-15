#!/bin/bash

cpee_home="$HOME/.cpee"

cpee_checkout(){
	echo -n "Which one do you want to checkout? [md5] : "
	read md5
	md5_len=${#md5}

	checkout_path=""
	for dir in $(ls -t $cpee_home) ; do

		if [[ ! -d $cpee_home/$dir ]]; then
			continue
		fi

		pushd $cpee_home/$dir > /dev/null

		for file in $(ls | grep -v log | grep -v path) ; do

			md5_file=$(md5sum $file | awk '{print $1}')
			if [[ $md5 = ${md5_file:0:$md5_len} ]] ; then
				checkout_path="$cpee_home/$dir/$file"
				break
			fi
		done

		popd > /dev/null

		if [[ -n $checkout_path ]] ; then
			cp -i --preserve $checkout_path .
			break
		fi
	done
}

update_log_cache(){
	echo -e "$1" > $cpee_home/logcache
}

read_log_cache(){
	if [[ -f $cpee_home/logcache ]] ; then
		cat $cpee_home/logcache
	else
		echo ""
	fi
}

cpee_read(){
	# $1 may be blank or subcommand
	sub=$1

	if [[ -f $cpee_home/logcache ]] ; then
		logdate=$(head -n1 $cpee_home/logcache)
		logdatesec=$(date --date="${logdate:18}" +%s)
	else
		logdatesec=0
	fi

	for dir in $(ls -t $cpee_home) ; do

		if [[ ! -d $cpee_home/$dir ]]; then
			continue
		fi

		datetime=$dir

		if [[ ${#datetime} -eq 14 ]] ; then

			YYYYmmdd=${datetime:0:8}
			HH=${datetime:8:2}
			MM=${datetime:10:2}
			SS=${datetime:12:2}
			datestr=$(env LC_TIME=en_US.UTF-8 date --date="${YYYYmmdd} ${HH}:${MM}:${SS}")

			if [[ $sub != "head" ]] ; then
				datesec=$(date --date="${YYYYmmdd} ${HH}:${MM}:${SS}" +%s)
				if [[ $datesec -le $logdatesec ]] ; then
					break
				fi
			fi

		else
			continue
		fi

		pushd $cpee_home/$dir > /dev/null

		if [[ -f log ]]; then
			log="$(cat log)"
			if [[ $sub = "head_log" ]] ; then
				echo "$log"
				exit 0
			fi
		else
			log="* * *"
		fi

		# XXX This is to avoid returning the error log 
		#     when subcommand is not "head_log"
		if [[ ! -d $cpee_home/$dir ]]; then
			buf="${buf}### Include file under $cpee_home : $dir.\n\n"
			continue
		fi

		if [[ -f path ]]; then
			src_path=$(head -n1 path)
			dst_path=$(tail -n1 path)
		else
			src_path="* * *"
			dst_path="* * *"
		fi

		buf="${buf}\e[33mDate   :\e[0m $datestr\n"
		buf="${buf}\e[33mPath   :\e[0m $src_path -> $dst_path\n"
		buf="${buf}\e[33mComment:\e[0m $log\n" # may include LF
		buf="${buf}\e[33mFiles  : [md5 size name]\e[0m\n"

		for file in $(ls | grep -v log | grep -v path) ; do
			md5_file=$(md5sum $file | awk '{print $1}')
			file_size=$(ls -lh $file | awk '{print $5}')
			buf="${buf}    ${md5_file:0:16} $file_size $file\n"
		done

		buf="${buf}\n"

		popd > /dev/null

		if [[ $sub = "head" ]] ; then
			break
		fi
	done

	if [[ $sub != "head" ]] ; then
		buf="${buf}$(read_log_cache)"
	fi

	if [[ $sub = "read" ]]; then
		echo -en "$buf" | less -XR
	elif [[ $sub = "show" || $sub = "head" ]]; then
		echo -en "$buf"
	fi

	update_log_cache "$buf"
}

get(){
	echo $(grep $1 $cpee_home/config | awk -F '=' '{print $2}')
}

get_sum_size() {
	sum=0
	# "$@" keeps the list of complete full path of source files
	for file in "$@" ; do
		if echo "$file" | grep /$ > /dev/null; then
			continue
		fi
		line=$(ls -1sFk $file)
		ksize=$(echo $line | awk '{print $1}')
		size=${ksize:0:-1} # remove the last 'K'
		sum=$((sum + size))
	done
	echo $((sum / 1000))
}

get_abs_dirname(){
	if [[ -d "$1" ]] ; then
		pushd "$1" > /dev/null
		echo $(pwd)
		popd > /dev/null
	else
		dir="${1%/*}"
		if [[ -d "$dir" ]] ;then
			pushd "$dir" > /dev/null
			echo $(pwd)
			popd > /dev/null
		else
			echo $(pwd)
		fi
	fi
}

if [[ $# -eq 0 ]]; then

	# XXX Envirnoment variable is better than config file?
	if [[ -f $cpee_home/config ]]; then
		echo -n "Do you want to overwrite your config? [y/n] : "
		read overw
		if [[ ${overw:0:1} = 'y' ]]; then
			curmaxtime=$(get "MAXTIME")
			curmaxsize=$(get "MAXSIZE")
		else
			exit 1
		fi
	else
		mkdir -p ~/.cpee/
		touch $cpee_home/config
	fi

	echo -n "How many copies do you want to have at most? $curmaxtime : "
	read maxtime
	if [[ -z $maxtime ]]; then
		maxtime=$curmaxtime
	fi

	echo -n "How much files do you want to copy at once at most? [MB] $curmaxsize : "
	read maxsize
	if [[ -z $maxsize ]]; then
		maxsize=$curmaxsize
	fi

	echo "MAXTIME=$maxtime" > $cpee_home/config
	echo "MAXSIZE=$maxsize" >> $cpee_home/config

elif [[ $# -eq 1 ]]; then
	# Subcommand
	sub=$1
	case $sub in
	read|log|show|head)
		if [[ $sub = "log" ]]; then
			sub="read"
		fi
		cpee_read $sub
		;;
	checkout|take)
		sub="checkout"
		cpee_checkout
		;;
	*)
		:
		;;
	esac

else

	if [[ ! -f $cpee_home/config ]]; then
		echo "!!! You need to run this command without any argument first."
		exit 1
	fi

	while [ 1 ]; do
		echo -n "Leave log for this copy : "
		read log
		if [[ -n $log ]]; then
			break
		else
			echo "### Empty string is not allowed."
		fi
	done

	head_log=$(cpee_read "head_log")
	if [[ "$head_log" = "$log" ]]; then
		echo -n "### This log is just same as head. Enter to quit. 'OK' to go. : "
		read check
		if [[ -z $check ]] ; then
			exit 10
		fi
	fi

	maxtime=$(grep "MAXTIME" $cpee_home/config | awk -F '=' '{print $2}')
	if [[ $(ls -1 $cpee_home | wc -l) -gt $maxtime ]]; then
		echo "!!! Reached to the maximum limit of the number of copies. Aborted."
		exit 1
	fi

	sum_size=$(get_sum_size "${@:1:$num_src}")
	maxsize=$(grep "MAXSIZE" $cpee_home/config | awk -F '=' '{print $2}')
	if [[ $sum_size -gt $maxsize ]]; then
		echo "!!! Reached to the maximum limit of the size of files. Aborted."
		exit 1
	fi

	argc=$#
	num_src=$((argc-1))
	idx_dst=$((argc)) # The last one must be destination

	# Argument may include space in thier path
	#src_files="${@:1:$num_src}" # This will not keep the arguments which include space
	dst="${@:$idx_dst:1}"
	if [[ -d "$dst" ]] ; then
		dst_dir=1
	else
		dst_dir=0
	fi

	cp "$@" # cp "$1" "$2" ...
	cp_ret=$?
	if [[ $cp_ret -ne 0 ]] ; then
		echo -e "\e[34mFailed.\e[0m\n"
		exit $cp_ret
	else
		echo -e "\e[32mSuccess.\e[0m\n"
	fi

	cpee_dir=$cpee_home/`date +%Y%m%d%H%M%S`
	mkdir $cpee_dir

	src_repl=""
	for src in "${@:1:$num_src}" ; do
		if [[ ${src:0:1} == '-' ]]; then
			continue
		elif [[ -z $src_repl ]] ; then
			src_repl=$src
		fi
		cp --preserve "$src" "$cpee_dir"
	done

	if [[ -n "$log" ]] ; then
		echo "$log" > $cpee_dir/log
	fi

	echo $(get_abs_dirname "$src_repl") > $cpee_dir/path
	echo $(get_abs_dirname "$dst") >> $cpee_dir/path

	sync;sync;sync

	echo "Copy source dir  : $(dirname $src_repl)"
	for src in "${@:1:$num_src}" ; do
		if [[ ${src:0:1} == '-' ]]; then
			continue
		fi
		ls -l "$src"
	done

	echo ""

	echo "Copy destination : $dst"
	if [[ $dst_dir -eq 1 ]] ; then
		for src in "${@:1:$num_src}" ; do
			if [[ ${src:0:1} == '-' ]]; then
				continue
			fi
			ls -l "$dst/$(basename $src)"
		done
	else
		ls -l "$dst"
	fi

fi

