# typed: strict

module Critic::SomePkg::Real
  FOO = 1
end

Critic::SomePkg::RealConst = 2

# Not allowed
Test::Critic::SomePkg::SomeTestConst = 3

# Not allowed
SomeOtherNamespace::SomePkg::SomeConst = 4
