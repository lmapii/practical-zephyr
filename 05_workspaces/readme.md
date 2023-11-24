
- [Goals](#goals)
- [Prerequisites](#prerequisites)
- [Creating a workspace from scratch](#creating-a-workspace-from-scratch)
  - [Installing west](#installing-west)
  - [Adding a manifest](#adding-a-manifest)
  - [Initializing the workspace](#initializing-the-workspace)
  - [Adding projects](#adding-projects)
- [Createing a workspace from a repository](#createing-a-workspace-from-a-repository)
  - [Updating the manifest repository path](#updating-the-manifest-repository-path)
  - [Locally vs. remotely initialized workspaces](#locally-vs-remotely-initialized-workspaces)
- [West _topdir_ vs. editor workspaces](#west-topdir-vs-editor-workspaces)
- [Working on external dependencies](#working-on-external-dependencies)
- [Zephyr with West](#zephyr-with-west)
- [TODO:](#todo)
- [Summary](#summary)
- [Further reading](#further-reading)

## Goals

## Prerequisites

## Creating a workspace from scratch

> **Note:** Our goal is to migrate a _freestanding application_ to a _west workspace_. In the official documentation, you'll find this as a [T2 star topology, where the application is the manifest repository][zephyr-west-workspaces].

Let's start from scratch, without any tools installed. Ok, that might be a bit much - let's say you have at least [`git`](https://git-scm.com/), [`python`](https://www.python.org/) and [`pipenv`](https://pipenv.pypa.io) installed. You can, of course, use `pyenv` or install directly using `pip`, whatever works best for you!

```bash
$ mkdir workspace
$ cd workspace
```


### Installing west

We've been using [West][zephyr-west] a lot now to build, debug and flash our project. It comes bundled with the [nRF Connect SDK][nrf-connect-sdk], so until now we didn't have to install it at all. Without sourcing any paths, e.g., from a `setup.sh` script that we've created at the very beginning of this article series, I don't have _West_ installed:

```bash
$ west --version
zsh: command not found: west
```

The first thing to even get started is therefore installing West, which is available as a `python` package. Instead of following the [official documentation][zephyr-west-install] and installing _West_ globally, I'll be using `pipenv` to manage an environment for me:

```bash
$ pwd
$ path/to/workspace
$ mkdir app
$ cd app
$ touch Pipfile
```

In my `Pipfile`, I can simply add `west` as a `dev-package`, since I won't be needing it for any `python` application or extension:

```ini
[[source]]
url = "https://pypi.org/simple"
verify_ssl = true
name = "pypi"

[dev-packages]
autopep8 = "*"
pylint = "*"
west = "1.2"

[requires]
python_version = "3.11"
```

All that's left to do is to install the environment and activate the shell, and we have _West_ available:

```bash
workspace/app $ pip install pipenv
workspace/app $ pipenv install --dev
workspace/app $ pipenv shell
(app) workspace/app $ west --version
West version: v1.2.0
```


### Adding a manifest

We now want to use _West_ to handle our dependencies: We really want to move on from using a global installation that may or will change at any time, and instead have all of our dependencies managed in our _workspace_. Zephyr evolves fast and it is therefore very important to have fixed versions of all modules and even Zephyr's source code.

_West_ solves this by using a so called [_manifest_ file][zephyr-west-manifest], which is nothing else than a list of external dependencies - and then some. _West_ uses this manifest file to basically act like a package manager for your _C_ project, similar to what [`cargo`](https://doc.rust-lang.org/stable/cargo/) does with its `cargo.toml` files for [_Rust_](https://www.rust-lang.org/).

> **Note:** Please don't pin me down for comparing _West_ with _cargo_. I'm just trying to find some other description than "Swiss Army Knife".

_West_ can also be used if you're _not_ planning to create a Zephyr project. In fact, let's start with a manifest file that doens't include Zephyr at all. For that, we create the file `app/west.yml` in our `workspace` folder.

```bash
(app) workspace/app $ cd ../
(app) workspace $ tree
.
└── app
    ├── Pipfile
    ├── Pipfile.lock
    └── west.yml
```

The minimal content of our _manifest_ file is the following:

`app/west.yml`
```yaml
manifest:
  # lowest version of the manifest file schema that can parse this file’s data
  version: 0.8
```

Zephyr's [official documentation on West basics][zephyr-west-basics] calls the `workspace` folder the "_topdir_" and our _app_ folder should be the _manifest **repository**_: In an idiomatic workspace, the folder containing the manifest file is a direct sibling of the "topdir" or workspace root directory, and it is a `git` repository.

> **Note:** I'm using the term "idiomatic" since there are multiple ways of using _West workspaces_. Placing your manifest file into a folder that is a direct sibling of the workspace is a _convention_, but nothing stops you from placing your manifest file in a different folder, e.g., deeper within your folder tree. This does, however, have some drawbacks since some of the _West_ commands, e.g., `west init`, are currently not very customizable, e.g., initializing a workspace using a repository and the `-m` argument won't work out-of-the-box.
>
> Feel free to experiment with your current version of _West_. I'm sure there will be some changes in the future, but for now we focus on the intended or _idiomatic_ way of using _West_.

Let's initialize a repository for our `app` folder containing the manifest files:

```bash
(app) workspace/app $ git init
Initialized empty Git repository in /path/to/workspace/app/.git/

(app) workspace/app $ tree --dirsfirst -a -L 2 ../
../      # topdir
└── app  # manifest _repository_
    ├── .git
    ├── Pipfile
    ├── Pipfile.lock
    └── west.yml  # manifest _file_

(app) workspace/app $ git remote add origin git@github.com:lmapii/practical-zephyr-t2-empty-ws.git
(app) workspace/app $ touch .gitignore
(app) workspace/app $ git add --all
(app) workspace/app $ git commit -m "initial commit"
(app) workspace/app $ git push -u origin main
```

### Initializing the workspace

Having everything under version control, we can relax and start exploring. First, we finally _create_ the _West_ workspace using [`west init`][zephyr-west-basics-init-update]. There are two ways to initialize a _West_ workspace:

- **Locally**, using the `-l` or `--local` flag: This assumes that your manifest repository already exists on your filesystem, e.g., you already used `git clone` to populate the _topdir_.
- **Remotely**, by specifying the URL to the manifest repository using the argument `-m`. With this argument, _West_ clones the manifest repository into the _topdir_ for you before initializing the workspace.

There is no difference between the two methods except that _West_ also clones the repository when using the `-m` argument to pass the manifest repository URL.

Since our manifest repository exists already, let's create a workspace using the local manifest repository:

```bash
(app) workspace/app $ west init --local .
=== Initializing from existing manifest repository app
--- Creating /path/to/workspace/.west and local configuration file
=== Initialized. Now run "west update" inside /path/to/workspace.

(app) workspace/app $ tree --dirsfirst -a -L 2 ../
../
├── .west
│   └── config
└── app
    ├── .git
    ├── .gitignore
    ├── Pipfile
    ├── Pipfile.lock
    └── west.yml
```

When initializing a workspace, _West_ creates a `.west` directory in the _topdir_, which in turn contains a configuration file.

```bash
(app) workspace/app $ cat ../.west/config
```
```ini
[manifest]
path = app
file = west.yml
```

The location of the `.west` folder "marks" the _topdir_ and thus _West_ workspace root directory. Within this file, we can see that _West_ stores the location and name of the manifest file. Modifying this file - or any file within the `.west` folder -  is not recommended, since some of _West's_ commands might no longer work as expected.

Doesn't _"west config"_ sound familiar? If you've been following along this article series, you might remember that we've already encountered the `west config` command in the very first article. We've used it, e.g., to configure the _board_ so that we didn't have to pass it as argument to `west build` anymore.

Let's try this in our workspace and see how it affects `.west/config`:

```bash
(app) workspace/app $ west config build.board nrf52840dk_nrf52840
(app) workspace/app $ cat ../.west/config
```
```ini
[manifest]
path = app
file = west.yml

[build]
board = nrf52840dk_nrf52840
```

Having initialized a _West workspace_, `west config` by default uses this _local_ configuration file to store its configuration options. _West_ also supports storing configuration options globally or even system wide. Have a look at the  [official documentation][zephyr-west-config] in case you want to know more.

Let's get rid of the configuration option and finally run [`west update`][zephyr-west-basics-init-update] as suggested by the ouptut we got in our call to `west init`:

```bash
(app) workspace/app $ west config -d build.board
(app) workspace/app $ cd ../
(app) workspace $ west update
(app) workspace $
```

Huh, that was disappointing. `west update` didn't do anything - feel free to check your files, nothing changed! The reason for that is quite simple: With `west init` we already took care of initializing the workspace. Since we don't have any external dependencies to manage, we have no work to do for `west update`.


### Adding projects

In your manifest file, you can use the `projects` key to define all Git repositories that you want to have in your workspace. Let's say that we're planning to have more modern [doxygen](https://doxygen.nl/) documentation for our project and therefore want to add [Doxygen Awesome][doxygen-awesome] to our workspace. We can add this external dependency as entry in the `projects` key of our manifest file:

`app/west.yml`
```yaml
manifest:
  version: 0.8

  projects:
    - name: doxygen-awesome
      url: https://github.com/jothepro/doxygen-awesome-css
      # you can also use SSH instead of HTTPS:
      # url: git@github.com:jothepro/doxygen-awesome-css.git
      revision: main
      path: deps/utilities/doxygen-awesome-css
```

Every _project_ at least has a **unique** name. Typically, you'll also specify the `url` of the repository - but there are several options, e.g., you could use the [`remotes` key][zephyr-west-manifest-remotes] to specify a list of URLs and add specify the `remote` that is used for your project. You can find examples and a detailed explanation of all available options in the [official documentation for the `projects` key][zephyr-west-remotes].

With the `revision` key you can tell _West_ to checkout either a specific _branch_, _tag_ or commit hash. Without specifying the `revision`, _West_ currently defaults to the `master` branch, and since [Doxygen Awesome][doxygen-awesome] uses `main` for its development branch, we have to explicitly specify it.

The `path` key is also an optional key that tells _West_ the _relative_ path to the _topdir_ to use when cloning the project. Without specifying the `path`, _West_ uses the project's `name` as path. Notice that you're **not** allowed to specify a path that is outside of the _topdir_ and thus _West_ workspace.

With that project added, we finally have some work for `west update`:

```bash
(app) workspace $ west update
=== updating doxygen-awesome (deps/utilities/doxygen-awesome-css):
--- doxygen-awesome: initializing
Initialized empty Git repository in /path/to/workspace/deps/utilities/doxygen-awesome-css/.git/
--- doxygen-awesome: fetching, need revision main
...
HEAD is now at 8cea9a0 security: fix link vulnerable to reverse tabnabbing (#127)
```

Great, now _West_ finally populated our dependencies and we have [Doxygen Awesome][doxygen-awesome] available in the specified `path`:

```bash
(app) workspace $ tree --dirsfirst -a -L 3 --filelimit 8
.
├── .west
│   └── config
├── app
│   ├── .git  [10 entries exceeds filelimit, not opening dir]
│   ├── .gitignore
│   ├── Pipfile
│   ├── Pipfile.lock
│   └── west.yml
└── deps
    └── utilities
        └── doxygen-awesome-css
```

In our manifest file, we specified that we want to use the `main` branch for the `doxygen-awesome` project. This kind of defeates the purpose of using a manifest to create a stable workspace, since we'll always be checking out the latest version of the repository when using `west update`. Instead, you'll typically either specify a _tag_ or even a specific commit in the revision.

At the time of writing, the tag `v2.2.1` was the latest release available for `doxygen-awesome`, pointing at the commit with the shortened has `df83fbf`. Let's update the manifest to use the latest tag:

`app/west.yml`
```yaml
manifest:
  version: 0.8

  projects:
    - name: doxygen-awesome
      url: https://github.com/jothepro/doxygen-awesome-css
      revision: v2.2.1
      path: deps/utilities/doxygen-awesome-css
```

Running `west update` changes our dependency to the specified revision:

```bash
(app) workspace $  west update
=== updating doxygen-awesome (deps/utilities/doxygen-awesome-css):
Warning: you are leaving 2 commits behind, not connected to
any of your branches:

  8cea9a0 security: fix link vulnerable to reverse tabnabbing (#127)
  00a52f6 Makefile: install -tabs.js as well (#122)

If you want to keep them by creating a new branch, this may be a good time
to do so with:

 git branch <new-branch-name> 8cea9a0

HEAD is now at df83fbf fix rendering error in example class
```

Looks like the `main` branch was already two commits ahead of the specified revision!

We now know that _West_ takes care of all the projects that we're using. We can, in fact, delete the entire `deps` repository and run `west update` again, and it'll simply put it back into its specified state. This is also the reason why it makes sense to group external dependencies, e.g., into this `deps` folder: Whenever you're not working on this project anymore, you can simply delete `deps` and `.west` folders to save some disk space. Once you pick it up again, simply run `west init` and `west update` and you're ready to go.

This is a project structure that Mike Szczys presented in his talk ["Manifests: Project Sanity in the Ever-Changing Zephyr World"][zephyr-ds22-manifests] at the 2022 Zephyr Developer Summit.

Whatever project structure you're using, however, is entirely up to you and always subject to personal preference.



## Createing a workspace from a repository

In Zephyr's official documentation for _West_, the first call to `west init` uses the `-m` argument to specify the manifest repository URL. In the previous section we've seen how we can _create_ such a manifest repository and how to use `west init --local` to initialize the workspace _locally_.

Instead of initializing a workspace _locally_, let's use the manifest file and repository that we've created in the previous section and initialize it using its remote URL. Let's fire up a new terminal and get started right away:

```bash
$ mkdir workspace-m
$ cd workspace-m
workspace-m $ west init -m git@github.com:lmapii/practical-zephyr-t2-empty-ws.git
zsh: command not found: west
```

Oh, right. I'm out of my `python` environment and don't even have west installed. Let's fix that:

```bash
workspace-m $ pipenv --python 3.11
workspace-m $ pipenv shell

(workspace-m) workspace-m $ pipenv install --dev "west>=1.2"
(workspace-m) workspace-m $ west --version
West version: v1.2.0
```

Now, we can finally initialize the workspace:

```bash
(workspace-m) workspace-m $ west init -m git@github.com:lmapii/practical-zephyr-t2-empty-ws.git
west init -m git@github.com:lmapii/practical-zephyr-t2-empty-ws.git
=== Initializing in /path/to/workspace-m
--- Cloning manifest repository from git@github.com:lmapii/practical-zephyr-t2-empty-ws.git
Cloning into '/path/to/workspace-m/.west/manifest-tmp'...
...
--- setting manifest.path to practical-zephyr-t2-empty-ws.git
=== Initialized. Now run "west update" inside /path/to/workspace-m.

(workspace-m) workspace-m $ tree --charset=utf-8 --dirsfirst -a -L 2
.
├── .west
│   └── config
├── practical-zephyr-t2-empty-ws.git
│   ├── .git
│   ├── .gitignore
│   ├── Pipfile
│   ├── Pipfile.lock
│   └── west.yml
├── Pipfile
└── Pipfile.lock
```

> **Note:** If you're irritated about the duplicate files `Pipfile` and `Pipfile.lock`, I'm keeping them on purpose and we'll get back to "fixing" this later in this article.

Notice that we no longer specify the current directory in the call to `west init` using "`.`". In fact, the directory - optionally passed as last argument to `west init` - is interpreted differently when using the `--local` flag or the `-m` arguments:

With `--local`, the directory specified the path to the local manifest repository. Without the `--local` flag, the directory refoers to the _topdir_ and thus the folder in which to create the workspace (defaulting to the current working directory in this case). Awkward, but probably due to legacy reasons.

The filetree, however, isn't exactly what we wanted, or is it it? `west init` created a folder `practical-zephyr-t2-empty-ws.git` instead of the folder called `app` that we've had before. There's no way for _West_ to know that we want the manifest repository to use the folder name `app`, so it uses the repository's name instaed. How can we change that?


### Updating the manifest repository path

The manifest file uses the key `self` for configuring the manifest repository itself, meaning that all settings in the `self` key are only applied to the manifest repository. The key `self.path` can be used to specify the path that _West_ uses when cloning the manifest repository, relative to the _West_ workspace _topdir_.

Let's update the `west.yml` file in the folder that `west init` cloned for us as follows:

`practical-zephyr-t2-empty-ws.git/west.yml`
```yaml
manifest:
  version: 0.8

  self:
    path: app

  projects:
    - name: doxygen-awesome
      url: https://github.com/jothepro/doxygen-awesome-css
      # you can also use SSH instead of HTTPS:
      # url: git@github.com:jothepro/doxygen-awesome-css.git
      revision: main
      path: deps/utilities/doxygen-awesome-css
```

Don't forget to commit and push the change: We're passing a URL to `west init` so _West_ will obviously not pick up local changes during the workspace initialization.

To reinitialize the workspace, we **must** remove the `.west` folder, otherwise `west init` throws an error telling us that the workspace is already initialized:

```bash
(workspace-m) workspace-m $ west init -m git@github.com:lmapii/practical-zephyr-t2-empty-ws.git
FATAL ERROR: already initialized in /path/to/workspace-m, aborting.
  Hint: if you do not want a workspace there,
  remove this directory and re-run this command:

  /path/to/workspace-m/.west
```

Let's follow the instruction and reinitialize the workspace:

```bash
(workspace-m) workspace-m $ rm -rf .west
(workspace-m) workspace-m $ west init -m git@github.com:lmapii/practical-zephyr-t2-empty-ws.git
west init -m git@github.com:lmapii/practical-zephyr-t2-empty-ws.git
=== Initializing in /path/to/workspace-m
...
--- setting manifest.path to app
=== Initialized. Now run "west update" inside /path/to/workspace-m.

(workspace-m) workspace-m $ tree --charset=utf-8 --dirsfirst -a -L 2
.
├── .west
│   └── config
├── app
│   ├── .git
│   ├── .gitignore
│   ├── Pipfile
│   ├── Pipfile.lock
│   └── west.yml
├── practical-zephyr-t2-empty-ws.git
├── Pipfile
└── Pipfile.lock
```

Now, _West_ uses the `app` folder as specified in the `self.path` configuration in the manifest file. It didn't touch the `practical-zephyr-t2-empty-ws.git` folder that it cloned in the last step, we need to remove that ourselves. Now we can run `west update` and - except for our `Pipfile`s - we end up with the same workspace as before:

```bash
(workspace-m) workspace-m $ rm -rf practical-zephyr-t2-empty-ws.git
(workspace-m) workspace-m $ west update
=== updating doxygen-awesome (deps/utilities/doxygen-awesome-css):
--- doxygen-awesome: initializing
Initialized empty Git repository in /path/to/workspace-m/deps/utilities/doxygen-awesome-css/.git/
--- doxygen-awesome: fetching, need revision v2.2.1
...
HEAD is now at df83fbf fix rendering error in example class

(workspace-m) workspace-m $ tree --dirsfirst -a -L 3 --filelimit 8
.
├── .west
│   └── config
├── app
│   ├── .git  [10 entries exceeds filelimit, not opening dir]
│   ├── .gitignore
│   ├── Pipfile
│   ├── Pipfile.lock
│   └── west.yml
├── deps
│   └── utilities
│       └── doxygen-awesome-css
├── Pipfile
└── Pipfile.lock
```


### Locally vs. remotely initialized workspaces

What is the difference between a _West workspace_ that has been initialized locally using the `--local` flag, or remotely by passing the URL of the manifest repository? Thankfully, the short answer is _none_.

The only difference is that _West_ clones the repository for you and thus you essentially don't need to use `git clone` for _any_ of the repositories in your workspace: West takes care of this, "forget" about `git clone` if you like.

We can also see this in the configuration file in `.west/config`:

```bash
(workspace-m) workspace-m $ cat .west/config
```
```ini
[manifest]
path = app
file = west.yml
```

Just like for the locally initialized repository, the `[manifest]` section points at the manifest file in the `app` folder. Running `west update` therefore only checks the contents of the local manifest file. It won't try to pull new changes in the manifest repository and it also won't attempt to read the file from the remote.

If there were any changes to the manifest file in the repository, you'll have to `git pull` them in manually - which is a good thing. In fact, `west update` will never attempt to modify the manifest repository and also states this in the `--help` information for the `update` command:

> "This command does not alter the manifest repository's contents."



## West _topdir_ vs. editor workspaces


## Working on external dependencies


## Zephyr with West




## TODO:

move away from freestanding to west workspace

reason "why west" and not just cmake

maybe little outlook on threads and final words

## Summary

Stuff becomes pretty wild depending on the MCU, but that's not Zephyr's fault.

E.g., building an APP for something like the nRF53840 is no longer a "simple switch", since all of a sudden it's a multi CPU SoC, using OpenAMP, requiring child images, partition manager (this is now vendor specific) ... but still, Zephyr as such still works and migrating no longer means you need to start from scratch. build, test, analysis, configuration system, all still the same.

Application is always only as simple as the system architecture.

## Further reading


[doxygen-awesome]: https://github.com/jothepro/doxygen-awesome-css
[nrf-connect-sdk]: https://www.nordicsemi.com/Products/Development-software/nrf-connect-sdk
[golioth]: https://golioth.io/

[zephyr-west]: https://docs.zephyrproject.org/latest/develop/west/index.html
[zephyr-west-install]: https://docs.zephyrproject.org/latest/develop/west/install.html
[zephyr-west-basics]: https://docs.zephyrproject.org/latest/develop/west/basics.html
[zephyr-west-basics-init-update]: https://docs.zephyrproject.org/latest/develop/west/basics.html#west-init-and-west-update
[zephyr-west-manifest]: https://docs.zephyrproject.org/latest/develop/west/manifest.html
[zephyr-west-workspaces]: https://docs.zephyrproject.org/latest/develop/west/workspaces.html
[zephyr-west-config]: https://docs.zephyrproject.org/latest/develop/west/config.html
[zephyr-west-remotes]: https://docs.zephyrproject.org/latest/develop/west/manifest.html#projects
[zephyr-west-manifest-remotes]: https://docs.zephyrproject.org/latest/develop/west/manifest.html#remotes
[zephyr-west-manifest-rev]: https://docs.zephyrproject.org/latest/develop/west/workspaces.html#west-manifest-rev

[zephyr-ds22-manifests]: https://www.youtube.com/watch?v=PVhu5rg_SGY