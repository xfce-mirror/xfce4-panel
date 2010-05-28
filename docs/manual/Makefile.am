SUBDIRS = \
	figures \
	po

MALLARD_PAGES = \
	index.page \
	actions.page \
	add-new-items.page \
	applicationsmenu.page \
	clock.page \
	directorymenu.page \
	introduction.page \
	launcher.page \
	license.page \
	pager.page \
	preferences.page \
	separator.page \
	showdesktop.page \
	systray.page \
	tasklist.page \
	theming.page \
	windowmenu.page

LINGUAS = $(shell cd $(srcdir)/po && echo `/bin/ls *.po | sed 's,[.]po,,g'`)
ALL_LINGUAS = C $(LINGUAS)

# install the html files to the destdir
install-data-local:
	file="$(srcdir)/html/$(PACKAGE).css"; \
	if test -f "$$file"; then \
		dest="$(DESTDIR)$(docdir)/html"; \
		$(mkinstalldirs) "$$dest"; \
		$(INSTALL_DATA) "$$file" "$$dest/"; \
	fi; \
	for lang in $(ALL_LINGUAS); do \
		if test -d "$(srcdir)/html/$$lang/index.html"; then \
			dest="$(DESTDIR)$(docdir)/html/$$lang"; \
			$(mkinstalldirs) "$$dest"; \
			installfiles=`echo "$(srcdir)/html/$$lang/*"`; \
			for file in $$installfiles; do \
				echo "--- Installing $$file"; \
				$(INSTALL_DATA) "$$file" "$$dest/"; \
			done; \
		fi \
	done

# remove installed files from the destdir
uninstall-local:
	file="$(DESTDIR)$(docdir)/html/$(PACKAGE).css"; \
	if test -f "$$file"; then \
		rm -f "$$file"; \
	fi; \
	for lang in $(ALL_LINGUAS); do \
		dest="$(DESTDIR)$(docdir)/html/$$lang"; \
		if test -d "$$dest"; then \
			rm -f $$dest/*.html; \
		fi \
	done;

# remove generate html from the srcdir
clean-local:
	rm -rf "$(srcdir)/html"

if GENERATE_DOCUMENTATION

# regenerate po(t) files
update-po:
	rm -f "$(srcdir)/po/$(GETTEXT_PACKAGE).pot"; \
	$(XML2PO) -o "$(srcdir)/po/$(GETTEXT_PACKAGE).pot" $(MALLARD_PAGES); \
	for lang in $(LINGUAS); do \
		$(XML2PO) -u "$(srcdir)/po/$$lang.po" $(MALLARD_PAGES);
	done

# build the translated help files
doc-build.stamp:
	@chmod -R u+w "$(srcdir)"; \
	rm -rf "$(srcdir)/html $(srcdir)/.pages"; \
	echo; \
	echo "Creating CSS file for documentation"; \
	$(GNOME_DOC_TOOL) css -o "$(srcdir)/html/$(PACKAGE).css" "$(srcdir)/index.page"; \
	echo; \
	for lang in $(ALL_LINGUAS); do \
		echo "Generating $$lang documentation"; \
		mkdir -p "$(srcdir)/.pages/"; \
		if test x"$$lang" = x"C"; then \
			for page in $(MALLARD_PAGES); do \
				cp -t "$(srcdir)/.pages/" "$(srcdir)/$$page"; \
			done; \
		else \
			echo "  - Creating translated pages"; \
			pofile="$(srcdir)/po/$$lang.po"; \
			for page in $(MALLARD_PAGES); do \
				$(XML2PO) -l "$$lang" -p "$$pofile" -o "$(srcdir)/.pages/$$page" "$(srcdir)/$$page"; \
			done; \
		fi; \
		if test -f "$(srcdir)/.pages/index.page"; then \
			echo "  - Building HTML files"; \
			$(GNOME_DOC_TOOL) html -n -c "../$(PACKAGE).css" -o "$(srcdir)/html/$$lang/" "$(srcdir)/.pages/"; \
			chmod -R u+w "$(srcdir)/html/$$lang"; \
		else \
			echo "  - No mallard pages found, skipping HTML generation!"; \
		fi; \
		rm -rf "$(srcdir)/.pages" "$(srcdir)/html/$$lang/index.cache"; \
		echo; \
	done; \
	touch doc-build.stamp

dist-check-doc: all

else

update-po:
	@echo "*** You need to compile with --enable-gen-doc in order to make update-po"
	@false

doc-build.stamp:
	@if test ! -d "$(srcdir)/html"; then \
		echo; \
		echo "*** No generated documentation found. If you want to"; \
		echo "*** generate the (translated) documentation, run configure"; \
		echo "*** with --enable-gen-doc."; \
		echo; \
	fi

dist-check-doc:
	@echo "*** You need to compile with --enable-gen-doc in order to make dist"
	@false

endif

# always try to build (if compiling with --enable-gen-doc) or print
# a message if no html files were found in the tarball
all: doc-build.stamp

# include the css and html files in the dist
dist-hook: dist-check-doc doc-build.stamp
	file="$(srcdir)/html/$(PACKAGE).css"; \
	if test -f "$$file"; then \
		mkdir -p "$(distdir)/html"; \
		cp "$$file" "$(distdir)/html/"; \
	fi; \
	for lang in $(ALL_LINGUAS); do \
		if test -d "$(srcdir)/html/$$lang/index.html"; then \
			mkdir -p "$(distdir)/html/$$lang"; \
			cp -t "$(distdir)/html/$$lang/" $(srcdir)/html/$$lang/*; \
		fi \
	done

EXTRA_DIST = \
	$(MALLARD_PAGES)

CLEANFILES = \
	doc-build.stamp
