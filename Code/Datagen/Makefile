all: 
	gcc -o noisy_data noisy_data.c datagen.c -std=c99 -lm
	gcc -o clean_data clean_data.c datagen.c -std=c99 -lm
	./noisy_data CSV sst_noisy bst_noisy
	./clean_data CSV sst_clean bst_clean

noisy_data:
	gcc -o noisy_data noisy_data.c datagen.c -std=c99 -lm
	./noisy_data
	
clean_data:
	gcc -o clean_data clean_data.c datagen.c -std=c99 -lm
	./clean_data
	
clean:
	rm noisy_data clean_data
