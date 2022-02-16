<!-- omit in toc -->
# Contributing to Control Surface Integration Project (CSI)

First off, thanks for taking the time to contribute! 

All types of contributions are encouraged and valued. See the [Table of Contents](#table-of-contents) for different ways to help and details about how this project handles them. Please make sure to read the relevant section before making your contribution. It will make it a lot easier for us maintainers and smooth out the experience for all involved. The community looks forward to your contributions. 

<!-- omit in toc -->
## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [I Have a Question](#i-have-a-question)
- [I Want To Contribute](#i-want-to-contribute)
  - [Reporting Bugs](#reporting-bugs)
  - [Suggesting Enhancements](#suggesting-enhancements)
  - [How to Make a Code Contribution](#your-first-code-contribution)
  - [Improving The Documentation](#improving-the-documentation)
  - [Maintaining Surface Config Files](*maintaining-config-files)
- [Styleguides](#styleguides)
  - [Commit Messages](#commit-messages)


## Code of Conduct

This project and everyone participating in it is governed by the
[CSI Project Code of Conduct](CODE_OF_CONDUCT.md).
By participating, you are expected to uphold this code. 


## I Have a Question

> If you want to ask a question, we assume that you have read the available [Documentation](https://github.com/GeoffAWaddington/reaper_csurf_integrator/wiki) and searched the [Reaper forums](https://forum.cockos.com/forumdisplay.php?f=20).

If you can't find what you are looking for, please post a detailed question to the forums. 


## I Want To Contribute

> ### Legal Notice <!-- omit in toc -->
> When contributing to this project, you must agree that you have authored 100% of the content, that you have the necessary rights to the content and that the content you contribute may be provided under the project license.

### Reporting Bugs

<!-- omit in toc -->
#### Before Submitting a Bug Report

A good bug report shouldn't leave others needing to chase you up for more information. Therefore, we ask you to investigate carefully, collect information and describe the issue in detail in your report. Please complete the following steps in advance to help us fix any potential bug as fast as possible.

- Make sure that you are using the latest stable version.
- Determine if your bug is really a bug and not an error on your side e.g. using config files for a differnet version (Make sure that you have read the [documentation](https://github.com/GeoffAWaddington/reaper_csurf_integrator/wiki). If you are looking for support, you might want to check [this section](#i-have-a-question)).
- Collect information about the bug:
  - OS, Platform and Version (Windows, Linux, macOS, x86, ARM)
  - Relevant sections of your mst and zone files
  - Can you reliably reproduce the issue? 

<!-- omit in toc -->
#### How Do I Submit a Good Bug Report?

We use GitHub issues to track bugs and errors. If you run into an issue with the project:

- Open an [Issue](https://github.com/GeoffAWaddington/reaper_csurf_integrator/issues/new). (Since we can't be sure at this point whether it is a bug or not, we ask you not to talk about a bug yet and not to label the issue.)
- Explain the behavior you would expect and the actual behavior.
- Please provide as much context as possible and describe the *reproduction steps* that someone else can follow to recreate the issue on their own. This usually includes parts of your mst and zone files. For good bug reports you should isolate the problem and create a reduced test case.
- Provide the information you collected in the previous section.

Once it's filed:

- The project team will label the issue accordingly.
- A team member will try to reproduce the issue with your provided steps. If there are no reproduction steps or no obvious way to reproduce the issue, the team will ask you for those steps and mark the issue as `needs-repro`. Bugs with the `needs-repro` tag will not be addressed until they are reproduced.
- If the team is able to reproduce the issue, it will be marked `needs-fix`, as well as possibly other tags (such as `critical`), and the issue will be left to be [implemented by someone](#how-to-make-a-code-contribution).


### Suggesting Enhancements

This section guides you through submitting an enhancement suggestion for the CSI Project, **including completely new features and minor improvements to existing functionality**. Following these guidelines will help maintainers and the community to understand your suggestion and find related suggestions.

<!-- omit in toc -->
#### Before Submitting an Enhancement

- Make sure that you are using the latest stable version.
- Read the [documentation](https://github.com/GeoffAWaddington/reaper_csurf_integrator/wiki) carefully and find out if the functionality is already covered, maybe by an individual configuration.
- Perform a [search](https://github.com/GeoffAWaddington/reaper_csurf_integrator/issues) to see if the enhancement has already been suggested. If it has, add a comment to the existing issue instead of opening a new one.
- Find out whether your idea fits with the scope and aims of the project. It's up to you to make a strong case to convince the project's developers of the merits of this feature. Keep in mind that we want features that will be useful to the majority of our users and not just a small subset.

<!-- omit in toc -->
#### How Do I Submit a Good Enhancement Suggestion?

Enhancement suggestions are tracked as [GitHub issues](https://github.com/GeoffAWaddington/reaper_csurf_integrator/issues).

- Use a **clear and descriptive title** for the issue to identify the suggestion.
- Provide a **step-by-step description of the suggested enhancement** in as many details as possible.
- **Describe the current behavior** and **explain which behavior you expected to see instead** and why. At this point you can also tell which alternatives do not work for you.
- **Explain why this enhancement would be useful** to most CSI Project users. You may also want to point out the other projects that solved it better and which could serve as inspiration.


### How to Make a Code Contribution
Did you write a patch that fixes a bug?

- Open a new GitHub pull request with the patch.
- Ensure the PR description clearly describes the problem and solution. Include the relevant issue number if applicable.

Do you intend to add a new feature or change an existing one?

- Suggest your change in the [CSI Reaper Forums Thread](https://forum.cockos.com/showthread.php?t=183143).
- If the change aligns with the design principals of the project and would be helpful to a wide base of users you can open a pull request providing you have collected positive feedback about the change. GitHub issues are primarily intended for bug reports and fixes. 
- Project maintainers will add comments to the request for any needed changes or improvements. Once the code is up to the required standard it will be pulled into the main branch. Be sure to follow the style guides below and maintin a consistent coding style.


### Improving The Documentation

<!-- omit in toc -->
#### I want to help out with the documentation

If you want to add or make changes to the wiki documentation. Contact @GeoffWaddington or @Funkybot on the forums. Create your documentation in plain text or [Markdown](https://docs.github.com/en/get-started/writing-on-github/getting-started-with-writing-and-formatting-on-github/basic-writing-and-formatting-syntax) format.

### Maintaining Surface Config Files
- State your willingness to supply and update config files on the [CSI Reaper Forums Thread.](https://forum.cockos.com/showthread.php?t=183143)
- Make sure each file has the supported CSI version number in a comment at the top of the file.
- Your name or username should be in the comments at the top of the file to help identify who wrote it.
- Use comments liberally. Remember you are creating a starting point for someone who will likely need to modify the files for their own needs. Your example will serve as a "bridge" between the documentation and a config that will serve their needs.

## Styleguides
### Commit Messages

- Use the present tense ("Add feature" not "Added feature")
- Use the imperative mood ("Move cursor to..." not "Moves cursor to...")
- Limit the first line to 72 characters or less
- Reference issues and pull requests liberally after the first line


<!-- omit in toc -->
## Attribution
This guide is based on the **contributing-gen**. [Make your own](https://github.com/bttger/contributing-gen)!
