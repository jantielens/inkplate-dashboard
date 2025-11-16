# GitHub Configuration

This directory contains configuration files for GitHub features and automation.

## GitHub Copilot

### Custom Instructions

**File:** `copilot-instructions.md`

This file provides repository-wide custom instructions for GitHub Copilot, ensuring consistent and context-aware AI assistance across:

- **GitHub Copilot in VS Code** - Code completions and chat
- **GitHub Copilot coding agent** - Automated code changes on GitHub.com
- **GitHub Copilot Code Review** - AI-powered code review suggestions

### How It Works

When you use GitHub Copilot in this repository, it automatically reads the instructions from `copilot-instructions.md` to understand:
- Project structure and architecture
- Build requirements and commands
- Code change guidelines
- Testing and validation requirements
- Documentation standards

### Best Practices

The instructions file follows GitHub's official recommendations:
- Located at `.github/copilot-instructions.md` for repo-wide instructions
- Uses clear markdown structure with headings and examples
- Contains project-specific context and guidelines
- Kept under 1000 lines for optimal processing

### Documentation

For more information about GitHub Copilot custom instructions:
- [Official Documentation](https://docs.github.com/en/copilot/how-tos/configure-custom-instructions/add-repository-instructions)
- [Best Practices Guide](https://docs.github.com/en/copilot/tutorials/use-custom-instructions)

## Workflows

The `workflows/` directory contains GitHub Actions workflows for:
- Continuous integration (build and test)
- Version validation
- Automated releases
- Documentation deployment

See each workflow file for specific details.
