#include "rate_limiter.hpp"

namespace kaiten {

// Глобальный экземпляр rate limiter
RateLimiter global_rate_limiter(60, 1000, std::chrono::milliseconds(100));

} // namespace kaiten