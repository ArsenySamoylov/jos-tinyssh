compile: make-tinyssh.sh
	$(MAKE) -C libjos
	sh -e make-tinyssh.sh
clean:
	rm -rf build
