all: clean EnsembleClassifier.out

EnsembleClassifier.out: ensembleClassifier.o linearClassifier.o voter.o 
 


ensembleClassifier.o: ensembleClassifier.cpp
	g++ -std=c++11 ensembleClassifier.cpp -o EnsembleClassifier.out

linearClassifier.o: linearClassifier.cpp
	g++ -std=c++11 linearClassifier.cpp -o linearClassifier

voter.o: voter.cpp
	g++ -std=c++11 voter.cpp -o voter

clean:
		find . -type f | xargs touch
		rm -rf *o ass4