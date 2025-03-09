
// postMessage Validation
window.addEventListener('message', function(event) {
  if (event.origin !== 'https://example.com') {
    return; // Reject messages from untrusted origins
  }

  // Handle the message
  // ...
});

// Origin Verification
const allowedOrigins = ['https://example.com', 'https://subdomain.example.com'];
if (!allowedOrigins.includes(window.location.origin)) {
  throw new Error('Invalid origin');
}

// Content Security Policy
const meta = document.createElement('meta');
meta.httpEquiv = 'Content-Security-Policy';
meta.content = "default-src 'self'; script-src 'self' 'unsafe-inline' https://example.com";
document.head.appendChild(meta);

// XSS Prevention
const userInput = '<script>alert("XSS attack!");</script>';
const sanitizedInput = userInput.replace(/</g, '&lt;').replace(/>/g, '&gt;');
document.getElementById('output').innerHTML = sanitizedInput;

// CSRF Protection
const csrfToken = document.querySelector('meta[name="csrf-token"]').content;
fetch('/api/data', {
  method: 'POST',
  headers: {
    'Content-Type': 'application/json',
    'X-CSRF-Token': csrfToken
  },
  body: JSON.stringify({ data: 'example' })
})
  .then(response => {
    // Handle the response
    // ...
  })
  .catch(error => {
    // Handle the error
    // ...
  });
