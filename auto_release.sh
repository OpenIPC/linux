#!/bin/bash

repo_name='base'
project_name='linux'
web_site='http://172.16.6.251:8088/'
url=$web_site$repo_name"/"$project_name"/"
package_prefix=$project_name'_'
record_version=0
project_version=''

get_beta_seq()
{
    content=`curl $url`
    
    OLD_IFS="$IFS"
    IFS="<>"
    
    array=($content)
    
	for var in ${array[@]}
    do
    	if [[ $var =~ 'beta' ]] ; then
    		if [[ ! $var =~ 'href' ]] ; then
    			if [[ $var =~ $project_version ]] ; then
    	            version_info=${var#*beta}
					if test ${#version_info} -eq 8 ; then
    				    version=${version_info:0:1}
					elif test ${#version_info} -eq 9 ; then
    				    version=${version_info:0:2}
					fi
    				if [[ $version -gt $record_version ]] ; then
						echo "version="$version
    					record_version=$version
    				fi
    			fi
    		fi
    	fi
    done

	IFS=$OLD_IFS
	record_version=$(($record_version+1))
}

usage()
{
	echo "Usage : ./auto_release.sh <Version>"
	exit
}

release_linux_kernel()
{
    package_name=$project_name"_"$project_version"-beta"$record_version".tar.gz"
    
    if [ -e $package_name ] ; then
    	rm -f $package_name
    fi

	cd ../
	tar cvfz $package_name --exclude=$project_name/auto_release.sh --exclude=$project_name/.git $project_name/*
    
    `curl "ftp://172.16.6.251/$repo_name/$project_name/$package_name" -u "release:123456" -T "$package_name"`
    
    rm -f $package_name
	cd linux
}

if [ $# -ne 1 ]  ; then
	usage
fi

project_version=$1
get_beta_seq

release_linux_kernel
