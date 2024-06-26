PRECOMPILIED := pre-compilied

compile: make-tinyssh.sh
	$(MAKE) -C libjos
ifeq (,$(wildcard $(PRECOMPILIED)))
		sh -e make-tinyssh.sh $(PRECOMPILIED)
else
		sh -e make-tinyssh.sh
endif
clean:
	rm -rf build
	rm -rf $(PRECOMPILIED)
