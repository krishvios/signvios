echo "Setting up fastlane dependency for ruby"
source ~/.zshrc

# Read versions from .tool-versions file
RUBY_VERSION=$(grep ruby ../.tool-versions | awk '{print $2}')

echo "Installing Ruby version: $RUBY_VERSION"
asdf plugin add ruby 2>/dev/null || echo "Ruby plugin already installed"
asdf install ruby $RUBY_VERSION
ruby -v

# bundle install
echo "Installing fastlane dependencies"
bundle install
