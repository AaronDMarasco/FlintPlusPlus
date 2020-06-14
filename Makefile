# Top-level Makefile

# Arbitrary version number:
VERSION := 0.5

define HELP
Flint++ $(VERSION) top-level Makefile options:

dist    - create tarball
rpm     - create RPM
manpage - regenerate man page (requires asciidoc)
clean   - clean tarballs and RPMs

For other options, try "make help" in flint subdirectory

endef

.PHONY: help
help: $(and $(filter help,$(MAKECMDGOALS)),$(info $(HELP))) $(or $(MAKECMDGOALS),$(info $(HELP)))
	@false

.PHONY: clean
clean:
	@rm -vrf flint++.tar flint++*.rpm

# If you're looking here because your tarball is broken, I "cheat" and only include files in HEAD; you need to ensure
# they've been checked into the repo at least once.
.PHONY: dist
.SILENT: dist
dist:
	git ls-tree -r --name-only -z HEAD | tar --null --files-from=- --owner=0 --group=0 --transform 's/^/flint++-$(VERSION)\//' -cJf flint++.tar
	ls -halF flint++.tar
	$(and $(VERBOSE),tar tvf flint++.tar)

.PHONY: rpm
.SILENT: rpm
# This changes every 6 minutes which is enough for updated releases (snapshots).
timestamp := $(shell printf %05d $(shell expr `date -u +"%s"` / 360 - 4422363))
git_hash := $(shell h=`(git tag --points-at HEAD | head -n1) 2>/dev/null`; \
              [ -z "$$h" ] && h=`git rev-list --max-count=1 HEAD`; echo $$h)
RPM_TEMP := $(CURDIR)/rpmbuild-tmpdir
rpm: dist
	rm -rf $(RPM_TEMP)
	install -v -D --target-directory $(RPM_TEMP)/SOURCES/ flint++.tar
	rpmbuild -ba \
  --define="RPM_VERSION $(VERSION)" \
  --define="RPM_HASH    $(git_hash)" \
  --define="COMMIT_TAG  _$(timestamp)" \
  --define="_topdir     $(RPM_TEMP)" \
  packaging/flint++.spec
	find $(RPM_TEMP)/ -name '*.rpm' | xargs cp -v --target-directory=.

.PHONY: manpage
.SILENT: manpage
manpage: flint++.1

.SILENT: flint++.1
flint++.1: manpage.txt
	type -t a2x &>/dev/null || (echo "Could not find a2x!" && false)
	type -t asciidoc &>/dev/null || (echo "Could not find asciidoc!" && false)
	type -t xmllint &>/dev/null || (echo "Could not find xmllint!" && false)
	type -t xsltproc &>/dev/null || (echo "Could not find xsltproc!" && false)
	rm -rf $@
	a2x --format=manpage -v $^
