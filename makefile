project_name=practical-zephyr

builder-build :
	docker build -f builder.Dockerfile -t $(project_name)-builder:latest .

builder-run :
	docker run \
		--rm \
		-it \
		--platform linux/amd64 \
		--workdir /workspaces/practical-zepyr \
		--mount type=bind,source="$$(pwd)",target=/workspaces/practical-zepyr \
		$(project_name)-builder:latest \
		/bin/bash
