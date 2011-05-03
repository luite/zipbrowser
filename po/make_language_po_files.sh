# generate the language specific po files
for i in `cat LINGUAS |grep --invert-match "^#"`; 
do 
    if [ ! -e $i.po ]
    then
        msginit --input="`ls -1 *.pot`" --locale="$i" --no-translator
        msgconv $i.po -t UTF-8 -o $i.po
    else
        echo "Skipping $i.po (already exists)"
    fi
done

for po in zh*.po
do
    if [ -e $po ]
    then
        sed -i -e 's/nplurals=INTEGER/nplurals=1/' -e 's/plural=EXPRESSION/plural=0/' $po
    fi
done
