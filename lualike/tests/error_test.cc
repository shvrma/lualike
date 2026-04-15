#include "lualike/error.h"

#include <gtest/gtest.h>

#include <stdexcept>

namespace lualike::error {

TEST(ErrorTest, ChainsMessagesAndKeepsPrettySourceRendering) {
  const auto err = Error::Context("invalid number", token::SourceSpan{7, 11})
                       .Wrap("while lexing expression")
                       .AttachSourceText("return 1..2");

  EXPECT_EQ(err.Messages().size(), 2);
  EXPECT_EQ(err.what(), std::string("while lexing expression: invalid number"));

  const auto pretty = err.RenderPretty();
  EXPECT_NE(pretty.find("Source context:"), std::string::npos);
  EXPECT_NE(pretty.find("^^^^"), std::string::npos);
  EXPECT_NE(pretty.find("invalid number"), std::string::npos);
}

TEST(ErrorTest, DeepCopiesErrorChains) {
  const auto original = Error::Context("failed to evaluate addition",
                                       token::SourceSpan{7, 15})
                            .Wrap("while interpreting return")
                            .AttachSourceText("return true + 1");

  const auto copy = original;

  EXPECT_EQ(copy.what(), original.what());
  EXPECT_EQ(copy.RenderPretty(), original.RenderPretty());
}

TEST(ErrorTest, PreservesForeignExceptions) {
  try {
    throw std::runtime_error("boom");
  } catch (...) {
    const auto err = Error::FromCurrentException("interpreter crashed")
                         .Wrap("while running script");

    EXPECT_EQ(err.what(),
              std::string("while running script: interpreter crashed: boom"));
    EXPECT_NE(err.RenderPlain().find("foreign exception: boom"),
              std::string::npos);
  }
}

}  // namespace lualike::error
