%.d: %.c
	$(CC) $(CFLAGS) -MM -MT "$@ $(@:.d=.o)" $< -o $@

%.d: %.cpp
	$(CXX) $(CXXFLAGS) -MM -MT "$@ $(@:.d=.o)" $< -o $@

.PHONY: clean-deps-graph
clean-deps-graph:
	@rm -f $(DEPS)

include $(DEPS)
