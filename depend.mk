#!/usr/bin/env -S gmake depend
# vim: set noexpandtab:

.PHONY: depend
depend: $(DEPS) $(SRCS) $(HDRS)
	echo > ./.depend
	for src in $(SRCS) tests/main.cc; do \
		dir="$$(dirname $$src)"; \
		src="$$(basename $$src)"; \
		if [[ $$src == *.cpp ]]; then \
			objdir="$${dir/src/obj}"; \
			echo "$(CXX) $(CPPFLAGS) -MM -MMD -MP -MF - \
				-MT $$objdir/$${src/.cpp/.o} $$dir/$$src >> ./.depend"; \
			$(CXX) $(CPPFLAGS) -MM -MMD -MP -MF - \
				-MT $$objdir/$${src/.cpp/.o} $$dir/$$src >> ./.depend || true ; \
		elif [[ $$src == *.c ]]; then \
			objdir="$${dir/src/obj}"; \
			echo "$(CC) $(CFLAGS) -MM -MMD -MP -MF - \
				-MT $$objdir/$${src/.c/.o} $$dir/$$src >> ./.depend"; \
			$(CC) $(CFLAGS) -MM -MMD -MP -MF - \
				-MT $$objdir/$${src/.c/.o} $$dir/$$src >> ./.depend || true ; \
		else continue; \
		fi; \
	done

include .depend
