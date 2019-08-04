#!/bin/bash

# all search commands
echo "Total number of keywords searched:" 
echo $(grep search  ../log/*.txt | cut -d , -f2 | sort | uniq | wc -l;)

# all diferent keywords
cut -d ' ' -f6 ../log/*.txt | sort | uniq | sed 's/.$//' > words

#the number of distinct file keyword was found
while read in;do grep "$in"  ../log/*.txt | cut -d , -f2 | sort | uniq | wc -l;done < words > tf

# merge the two texts
paste words tf > results

#sort em
sort -k 2 results > final

#mincount
echo "Keyword min frequently found:"
echo $(head -n 1 final)

#maxcount
echo "Keyword max frequently found:"
echo $(tail -n 1 final)
