filter_stable:
	gcc filter_stable.c datagen.c update_alignment.c -std=c99 -lm -o filter_stable
	./filter_stable

filter_unstable:
	gcc filter.c datagen.c update_alignment.c -std=c99 -lm -o filter_unstable
	./filter_unstable

filter_split:
	gcc filter_split.c datagen.c -std=c99 -lm -o filter_split
	
clean:
	rm -f filter_stable filter_unstable
