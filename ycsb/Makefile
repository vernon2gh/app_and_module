all: ycsb-0.17.0
	@echo "Please running ./test.sh"

ycsb-0.17.0:
	@echo "Start download YCSB..."
	curl -O --location https://github.com/brianfrankcooper/YCSB/releases/download/0.17.0/ycsb-0.17.0.tar.gz
	tar -zxvf ycsb-0.17.0.tar.gz

clean:
	sudo rm -fr data ycsb*
