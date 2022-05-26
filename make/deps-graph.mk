%.d: %.c
	$(CC) $(CFLAGS) -MM -MT "$@ $(@:.d=.o)" $< -o $@

.PHONY: clean-deps-graph
clean-deps-graph:
	@rm -f $(DEPS)

include $(DEPS)
